/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by wuyuncheng on 9/7/21.
//

#include <falcon/algorithm/vertical/tree/forest_model.h>
#include <falcon/algorithm/vertical/tree/tree_builder.h>
#include <falcon/algorithm/vertical/tree/tree_model.h>
#include <falcon/common.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/operator/mpc/spdz_connector.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <cmath>
#include <falcon/utils/alg/tree_util.h>
#include <falcon/utils/logger/logger.h>
#include <glog/logging.h>
#include <iostream>

ForestModel::ForestModel() = default;

ForestModel::ForestModel(int m_tree_size, const std::string &m_tree_type) {
  tree_size = m_tree_size;
  // copy builder parameters
  if (m_tree_type == "classification") {
    tree_type = falcon::CLASSIFICATION;
  } else {
    tree_type = falcon::REGRESSION;
  }
  forest_trees.reserve(tree_size);
}

ForestModel::~ForestModel() = default;

ForestModel::ForestModel(const ForestModel &forest_model) {
  tree_size = forest_model.tree_size;
  tree_type = forest_model.tree_type;
  forest_trees = forest_model.forest_trees;
}

ForestModel &ForestModel::operator=(const ForestModel &forest_model) {
  tree_size = forest_model.tree_size;
  tree_type = forest_model.tree_type;
  forest_trees = forest_model.forest_trees;
}

void ForestModel::predict(
    Party &party, const std::vector<std::vector<double>> &predicted_samples,
    int predicted_sample_size, EncodedNumber *predicted_labels) {
  /// the prediction workflow is as follows:
  ///     step 1: the parties compute a prediction for each tree for each sample
  ///     step 2: the parties convert the predictions to secret shares and send
  ///     to mpc to compute mode step 3: the parties convert the secret shared
  ///     results back to ciphertext

  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);

  // init predicted forest labels to record the predictions, the first dimension
  // is the number of trees in the forest, and the second dimension is the
  // number of the samples in the predicted dataset
  auto **predicted_forest_labels = new EncodedNumber *[tree_size];
  for (int tree_id = 0; tree_id < tree_size; tree_id++) {
    predicted_forest_labels[tree_id] = new EncodedNumber[predicted_sample_size];
  }

  // compute predictions
  for (int tree_id = 0; tree_id < tree_size; tree_id++) {
    // compute predictions for each tree in the forest
    forest_trees[tree_id].predict(party, predicted_samples,
                                  predicted_sample_size,
                                  predicted_forest_labels[tree_id]);
  }
  int cipher_precision = abs(predicted_forest_labels[0][0].getter_exponent());
  log_info("cipher_precision = " + std::to_string(cipher_precision));

  // if classification, needs to communicate with mpc
  // otherwise, compute average of the prediction
  if (tree_type == falcon::REGRESSION) {
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int i = 0; i < predicted_sample_size; i++) {
        auto *aggregation = new EncodedNumber[1];
        aggregation[0].set_double(phe_pub_key->n[0], 0.0, cipher_precision);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random, aggregation[0],
                           aggregation[0]);
        // aggregate the predicted labels for sample i
        for (int tree_id = 0; tree_id < tree_size; tree_id++) {
          djcs_t_aux_ee_add(phe_pub_key, aggregation[0], aggregation[0],
                            predicted_forest_labels[tree_id][i]);
        }
        // compute average of the aggregation
        EncodedNumber enc_tree_size;
        enc_tree_size.set_double(phe_pub_key->n[0], (1.0 / tree_size),
                                 cipher_precision);
        djcs_t_aux_ep_mul(phe_pub_key, predicted_labels[i], aggregation[0],
                          enc_tree_size);
        delete[] aggregation;
      }
    }
    broadcast_encoded_number_array(party, predicted_labels,
                                   predicted_sample_size, ACTIVE_PARTY_ID);
  } else {
    std::vector<int> public_values;
    std::vector<double> private_values;
    public_values.push_back(tree_size);
    public_values.push_back(predicted_sample_size);

    // convert the predicted_forest_labels into secret shares
    // the structure is one-dimensional vector, [tree_1 * sample_size] ...
    // [tree_n * sample_size]
    std::vector<std::vector<double>> forest_label_secret_shares;
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      std::vector<double> tree_label_secret_shares;
      ciphers_to_secret_shares(party, predicted_forest_labels[tree_id],
                               tree_label_secret_shares, predicted_sample_size,
                               ACTIVE_PARTY_ID, cipher_precision);
      forest_label_secret_shares.push_back(tree_label_secret_shares);
    }
    // pack to private values for sending to mpc parties
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      for (int i = 0; i < predicted_sample_size; i++) {
        private_values.push_back(forest_label_secret_shares[tree_id][i]);
      }
    }

    falcon::SpdzTreeCompType comp_type = falcon::RF_LABEL_MODE;

    // communicate with spdz parties and receive results to compute labels
    // first send computation type, tree type, class num
    // then send private values
    std::promise<std::vector<double>> promise_values;
    std::future<std::vector<double>> future_values =
        promise_values.get_future();
    std::thread spdz_pruning_check_thread(
        spdz_tree_computation, party.party_num, party.party_id,
        party.executor_mpc_ports, party.host_names, public_values.size(),
        public_values, private_values.size(), private_values, comp_type,
        &promise_values);
    std::vector<double> predicted_sample_shares = future_values.get();
    spdz_pruning_check_thread.join();

    // convert the secret shares to ciphertext, which is the predicted labels
    secret_shares_to_ciphers(party, predicted_labels, predicted_sample_shares,
                             predicted_sample_size, ACTIVE_PARTY_ID,
                             cipher_precision);
  }

  // free memory
  djcs_t_free_public_key(phe_pub_key);
  for (int tree_id = 0; tree_id < tree_size; tree_id++) {
    delete[] predicted_forest_labels[tree_id];
  }
  delete[] predicted_forest_labels;
}

void ForestModel::predict_proba(
    Party &party, const std::vector<std::vector<double>> &predicted_samples,
    int predicted_sample_size, EncodedNumber **predicted_labels) {
  log_info("[ForestModel.predict_proba] begin to predict forest model");
  if (tree_type == falcon::REGRESSION) {
    log_info("[ForestModel.predict_proba] regression tree");
    predict(party, predicted_samples, predicted_sample_size,
            predicted_labels[0]);
    return;
  }

  if (tree_type == falcon::CLASSIFICATION) {
    log_info("[ForestModel.predict_proba] classification tree");
    // the prediction workflow is as follows:
    //     step 1: the parties compute a prediction for each tree for each
    //     sample step 2: the parties convert the predictions to secret shares
    //     and send to mpc to compute mode step 3: the parties convert the
    //     secret shared results back to ciphertext

    // retrieve phe pub key and phe random
    djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
    party.getter_phe_pub_key(phe_pub_key);

    // init predicted forest labels to record the predictions, the first
    // dimension is the number of trees in the forest, and the second dimension
    // is the number of the samples in the predicted dataset
    auto **predicted_forest_labels = new EncodedNumber *[tree_size];
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      predicted_forest_labels[tree_id] =
          new EncodedNumber[predicted_sample_size];
    }

    // compute predictions
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      // compute predictions for each tree in the forest
      forest_trees[tree_id].predict(party, predicted_samples,
                                    predicted_sample_size,
                                    predicted_forest_labels[tree_id]);
    }

    // TODO: currently for lime baseline comparison, decrypt the predicted
    // labels
    int class_num = forest_trees[0].class_num;
    auto **decrypted_forest_labels = new EncodedNumber *[tree_size];
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      decrypted_forest_labels[tree_id] =
          new EncodedNumber[predicted_sample_size];
    }
    for (int i = 0; i < tree_size; i++) {
      collaborative_decrypt(party, predicted_forest_labels[i],
                            decrypted_forest_labels[i], predicted_sample_size,
                            ACTIVE_PARTY_ID);
    }
    // active party compute prediction probabilities and encrypt
    if (party.party_type == falcon::ACTIVE_PARTY) {
      std::vector<std::vector<double>> dec_predicted_labels;
      for (int i = 0; i < tree_size; i++) {
        std::vector<double> dec_predicted_labels_tree_i;
        for (int j = 0; j < predicted_sample_size; j++) {
          double d;
          decrypted_forest_labels[i][j].decode(d);
          dec_predicted_labels_tree_i.push_back(d);
        }
        dec_predicted_labels.push_back(dec_predicted_labels_tree_i);
      }
      // compute probabilities
      std::vector<std::vector<double>> samples_pred_prob;
      for (int j = 0; j < predicted_sample_size; j++) {
        std::vector<double> pred;
        for (int i = 0; i < tree_size; i++) {
#ifdef DEBUG
          if (j == 0) {
            log_info("[ForestModel.predict_proba] print the first sample's "
                     "tree predicted labels");
            log_info("[ForestModel.predict_proba] dec_predicted_labels[" +
                     std::to_string(i) +
                     "][0] = " + std::to_string(dec_predicted_labels[i][j]));
          }
#endif
          pred.push_back(dec_predicted_labels[i][j]);
        }
        std::vector<double> prob = rf_pred2prob(class_num, pred);
#ifdef DEBUG
        if (j == 0) {
          log_info("[ForestModel.predict_proba] print the first sample's class "
                   "probabilities");
          for (int i = 0; i < prob.size(); i++) {
            log_info("[ForestModel.predict_proba] prob[" + std::to_string(i) +
                     "]" + std::to_string(prob[i]));
          }
        }
#endif
        samples_pred_prob.push_back(prob);
      }
      // encrypt
      for (int i = 0; i < predicted_sample_size; i++) {
        for (int j = 0; j < class_num; j++) {
          // for temporarily handling the extreme case that the probability is 0
          if (samples_pred_prob[i][j] == 0.0) {
            samples_pred_prob[i][j] = 1e-3;
          }
          predicted_labels[i][j].set_double(phe_pub_key->n[0],
                                            samples_pred_prob[i][j],
                                            2 * PHE_FIXED_POINT_PRECISION);
          djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                             predicted_labels[i][j], predicted_labels[i][j]);
        }
      }
    }

    // active party broadcast the predicted labels
    for (int i = 0; i < predicted_sample_size; i++) {
      broadcast_encoded_number_array(party, predicted_labels[i], class_num,
                                     ACTIVE_PARTY_ID);
    }

    // free memory
    djcs_t_free_public_key(phe_pub_key);
    for (int tree_id = 0; tree_id < tree_size; tree_id++) {
      delete[] predicted_forest_labels[tree_id];
      delete[] decrypted_forest_labels[tree_id];
    }
    delete[] predicted_forest_labels;
    delete[] decrypted_forest_labels;
  }
}