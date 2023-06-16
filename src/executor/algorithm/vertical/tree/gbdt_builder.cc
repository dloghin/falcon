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
// Created by wuyuncheng on 31/7/21.
//

#include <falcon/algorithm/vertical/tree/gbdt_builder.h>
#include <falcon/algorithm/vertical/tree/gbdt_loss.h>
#include <falcon/algorithm/vertical/tree/gbdt_model.h>
#include <falcon/model/model_io.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/logger/log_alg_params.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/math/math_ops.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/tree_converter.h>

#include <falcon/utils/logger/logger.h>
#include <glog/logging.h>

#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <iomanip>
#include <map>
#include <random>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/types.h>

#include <Networking/ssl_sockets.h>

GbdtBuilder::GbdtBuilder() = default;

GbdtBuilder::~GbdtBuilder() = default;

GbdtBuilder::GbdtBuilder(const GbdtParams &gbdt_params,
                         std::vector<std::vector<double>> m_training_data,
                         std::vector<std::vector<double>> m_testing_data,
                         std::vector<double> m_training_labels,
                         std::vector<double> m_testing_labels,
                         double m_training_accuracy, double m_testing_accuracy)
    : ModelBuilder(std::move(m_training_data), std::move(m_testing_data),
                   std::move(m_training_labels), std::move(m_testing_labels),
                   m_training_accuracy, m_testing_accuracy) {
  n_estimator = gbdt_params.n_estimator;
  loss = gbdt_params.loss;
  learning_rate = gbdt_params.learning_rate;
  subsample = gbdt_params.subsample;
  dt_param = gbdt_params.dt_param;
  int tree_size;
  // if regression or binary classification, tree size = n_estimator
  if (dt_param.tree_type == "classification" && dt_param.class_num > 2) {
    tree_size = n_estimator * dt_param.class_num;
  } else {
    tree_size = n_estimator;
  }
  gbdt_model = GbdtModel(tree_size, dt_param.tree_type, n_estimator,
                         dt_param.class_num, learning_rate);
  tree_builders.reserve(tree_size);
  local_feature_num = (int)training_data[0].size();
}

void GbdtBuilder::train(Party party) {
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);

  // two branches for training gbdt model: regression and classification
  log_info("************ Begin to train the GBDT model ************");
  // required by spdz connector and mpc computation
  // bigint::init_thread();
  if (gbdt_model.tree_type == falcon::REGRESSION) {
    train_regression_task(party);
  } else {
    train_classification_task(party);
  }
  log_info("End train the GBDT model");
  struct timespec finish;
  clock_gettime(CLOCK_MONOTONIC, &finish);
  double consumed_time = (double)(finish.tv_sec - start.tv_sec);
  consumed_time += (double)(finish.tv_nsec - start.tv_nsec) / 1000000000.0;
  log_info("Training time = " + std::to_string(consumed_time));
}

void GbdtBuilder::distributed_train(const Party &party, const Worker &worker) {}

void GbdtBuilder::train_regression_task(Party party) {
  /// For regression, build the trees as follows:
  ///   1. init a dummy estimator with mean, compute raw_predictions as mean
  ///   2. for tree id 0 to num-1, init a decision tree, compute the encrypted
  ///      residuals between the original labels and raw_predictions
  ///   3. build a decision tree model by calling the train with encrypted
  ///      labels api in cart_tree.h
  ///   4. update the raw_predictions, using the predicted labels from the
  ///      current tree, and iterate for the next tree, until finished

  // check loss function
  if (loss != "ls") {
    log_error("The loss function is not supported, need to use the least "
              "square error loss function.");
    exit(EXIT_FAILURE);
  }
  // step 1
  // init the dummy estimator and compute raw_predictions:
  // for regression, using the mean label
  int sample_size = (int)training_data.size();
  auto *raw_predictions = new EncodedNumber[sample_size];
  // retrieve phe pub key
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // init loss function, for regression loss, class num is set to 1
  LeastSquareError least_square_error(gbdt_model.tree_type, 1);
  // get the initial encrypted raw_predictions, all parties obtain
  // raw_predictions
  least_square_error.get_init_raw_predictions(
      party, raw_predictions, sample_size, training_data, training_labels);
  // add the dummy mean prediction to the gbdt_model
  gbdt_model.dummy_predictors.emplace_back(least_square_error.dummy_prediction);
  // init the encrypted labels, only the active party can init
  auto *encrypted_true_labels = new EncodedNumber[sample_size];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    for (int i = 0; i < sample_size; i++) {
      encrypted_true_labels[i].set_double(phe_pub_key->n[0],
                                          training_labels[i]);
      djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                         encrypted_true_labels[i], encrypted_true_labels[i]);
    }
  }
  // active party broadcasts the encrypted true labels
  broadcast_encoded_number_array(party, encrypted_true_labels, sample_size,
                                 ACTIVE_PARTY_ID);

  // iteratively train the regression trees
  log_info("tree_size = " + std::to_string(gbdt_model.tree_size));
  for (int tree_id = 0; tree_id < gbdt_model.tree_size; ++tree_id) {
    log_info("------------- build the " + std::to_string(tree_id) +
             "-th tree -------------");
    // step 2
    // compute the encrypted residual, i.e., negative gradient
    auto *residuals = new EncodedNumber[sample_size];
    least_square_error.negative_gradient(
        party, encrypted_true_labels, raw_predictions, residuals, sample_size);
    log_info("negative gradient computed finished");
    auto *squared_residuals = new EncodedNumber[sample_size];
    square_encrypted_residual(party, residuals, squared_residuals, sample_size,
                              PHE_FIXED_POINT_PRECISION);
    // flatten the residuals and squared_residuals into one vector for calling
    // the train_with_encrypted_labels in cart_tree.h
    auto *flatten_residuals =
        new EncodedNumber[REGRESSION_TREE_CLASS_NUM * sample_size];
    for (int i = 0; i < sample_size; i++) {
      flatten_residuals[i] = residuals[i];
      flatten_residuals[sample_size + i] = squared_residuals[i];
    }
    log_info("compute squared residuals finished");
    // step 3
    // init a tree builder and train the model with flatten_residuals
    tree_builders.emplace_back(dt_param, training_data, testing_data,
                               training_labels, testing_labels,
                               training_accuracy, testing_accuracy);
    tree_builders[tree_id].train(party, flatten_residuals);
    log_info("tree builder = " + std::to_string(tree_id) + " train finished");
    // step 4
    // after training the model, update the terminal regions, for least
    // square error, only need to update the raw_predictions
    least_square_error.update_terminal_regions(
        party, tree_builders[tree_id], encrypted_true_labels, residuals,
        raw_predictions, sample_size, learning_rate, 0);
    log_info("update terminal regions and raw predictions finished");
    // save the trained regression tree model to gbdt_model
    gbdt_model.gbdt_trees.emplace_back(tree_builders[tree_id].tree);

    // free temporary variables
    delete[] residuals;
    delete[] squared_residuals;
    delete[] flatten_residuals;
  }
  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
  delete[] raw_predictions;
  delete[] encrypted_true_labels;
}

void GbdtBuilder::train_classification_task(Party party) {
  /// For classification, build the trees as follows:
  ///   1. if binary classification: apply the BinomialDeviance loss function,
  ///      where the number of estimator inside the loss function is set to 1,
  ///      first init a dummy estimator with probability = (#event / #total),
  ///      note that the corresponding get_init_raw_predictions = log(odds)
  ///      = log(p/(1-p), which will be updated in the following tree building
  ///      (1.1) then, for tree id 0 to num-1, init a decision tree, compute
  ///      the encrypted residuals between the original labels and
  ///      raw_predictions (1.2) build a regression tree model by calling the
  ///      train with encrypted labels api in cart_tree.h (1.3) update the
  ///      terminal regions, and update the raw_predictions, iterate for the
  ///      next tree, until finished
  ///   2. if multi-class classification: apply the MultinomialDeviance loss
  ///      function, where the number of estimator inside the loss function is
  ///      set to class_num, i.e., for each class, build n_estimator trees,
  ///      first init a dummy estimator with probability = (#event / #total)
  ///      for each class, and compute the raw_predictions
  ///      (2.1) for each class, convert to the one-hot encoding dataset, from
  ///      tree id 0 to num-1, init a decision tree, compute the encrypted
  ///      residuals between the original one-hot labels and raw_predictions
  ///      (2.2) build a regression tree model by calling the train with
  ///      encrypted labels api in cart_tree.h (2.3) update the terminal
  ///      regions, and update the raw_predictions, iterate for the next tree,
  ///      until finished

  // check loss function
  if (loss != "deviance") {
    log_error("The loss function is not supported, need to use the deviance "
              "loss function.");
    exit(EXIT_FAILURE);
  }
  log_info("Begin train a gbdt classification task");
  log_info("gbdt_model.class_num = " + std::to_string(gbdt_model.class_num));
  // retrieve phe pub key
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  int sample_size = (int)training_data.size();
  // check class_num, for binary classification, use BinomialDeviance loss
  // for multi-class classification, use MultinomialDeviance loss
  if (dt_param.class_num == 2) {
    // step 1
    // init the dummy estimator and compute raw_predictions:
    // for binary classification, using the log(odds) for the raw_predictions
    auto *raw_predictions = new EncodedNumber[sample_size];
    // init loss function, for regression loss, class num is set to 1
    BinomialDeviance binomial_deviance(gbdt_model.tree_type,
                                       dt_param.class_num);
    // get the initial encrypted raw_predictions, all parties obtain
    // raw_predictions
    binomial_deviance.get_init_raw_predictions(
        party, raw_predictions, sample_size, training_data, training_labels);
    // add the dummy mean prediction to the gbdt_model
    gbdt_model.dummy_predictors.emplace_back(
        binomial_deviance.dummy_prediction);
    // init the encrypted labels, only the active party can init
    auto *encrypted_true_labels = new EncodedNumber[sample_size];
    if (party.party_type == falcon::ACTIVE_PARTY) {
      for (int i = 0; i < sample_size; i++) {
        encrypted_true_labels[i].set_double(phe_pub_key->n[0],
                                            training_labels[i]);
        djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                           encrypted_true_labels[i], encrypted_true_labels[i]);
      }
    }
    // active party broadcasts the encrypted true labels
    broadcast_encoded_number_array(party, encrypted_true_labels, sample_size,
                                   ACTIVE_PARTY_ID);

    // iteratively train the regression trees
    log_info("tree_size = " + std::to_string(gbdt_model.tree_size));
    for (int tree_id = 0; tree_id < gbdt_model.tree_size; ++tree_id) {
      log_info("------------- build the " + std::to_string(tree_id) +
               "-th tree -------------");
      // step 2
      // compute the encrypted residual, i.e., negative gradient
      auto *residuals = new EncodedNumber[sample_size];
      binomial_deviance.negative_gradient(party, encrypted_true_labels,
                                          raw_predictions, residuals,
                                          sample_size);
      log_info("negative gradient computed finished");
      auto *squared_residuals = new EncodedNumber[sample_size];
      square_encrypted_residual(party, residuals, squared_residuals,
                                sample_size, PHE_FIXED_POINT_PRECISION);
      // flatten the residuals and squared_residuals into one vector for calling
      // the train_with_encrypted_labels in cart_tree.h
      auto *flatten_residuals =
          new EncodedNumber[REGRESSION_TREE_CLASS_NUM * sample_size];
      for (int i = 0; i < sample_size; i++) {
        flatten_residuals[i] = residuals[i];
        flatten_residuals[sample_size + i] = squared_residuals[i];
      }
      log_info("compute squared residuals finished");
      // step 3
      // init a tree builder and train the model with flatten_residuals
      // check gbdt task type, note that the gbdt type can be classification,
      // but all the trained decision trees are regression trees
      DecisionTreeParams classification_dt_params = dt_param;
      classification_dt_params.tree_type = "regression";
      classification_dt_params.class_num = REGRESSION_TREE_CLASS_NUM;
      tree_builders.emplace_back(classification_dt_params, training_data,
                                 testing_data, training_labels, testing_labels,
                                 training_accuracy, testing_accuracy);
      tree_builders[tree_id].train(party, flatten_residuals);
      log_info("tree builder = " + std::to_string(tree_id) + " train finished");
      // step 4
      // after training the model, update the terminal regions, for least
      // square error, only need to update the raw_predictions
      binomial_deviance.update_terminal_regions(
          party, tree_builders[tree_id], encrypted_true_labels, residuals,
          raw_predictions, sample_size, learning_rate, 0);
      log_info("update terminal regions and raw predictions finished");
      // save the trained regression tree model to gbdt_model
      gbdt_model.gbdt_trees.emplace_back(tree_builders[tree_id].tree);

      // free temporary variables
      delete[] residuals;
      delete[] squared_residuals;
      delete[] flatten_residuals;
    }
    delete[] raw_predictions;
    delete[] encrypted_true_labels;
  }

  // multi-class classification
  if (gbdt_model.class_num > 2) {
    // step 1
    // init the dummy estimator and compute raw_predictions: for multi-class
    // classification, using the log(odds) for the raw_predictions of each tree
    auto *raw_predictions =
        new EncodedNumber[sample_size * gbdt_model.class_num];
    // init loss function, for regression loss, class num is set to 1
    MultinomialDeviance multinomial_deviance(gbdt_model.tree_type,
                                             gbdt_model.class_num);
    // get the initial encrypted raw_predictions, all parties obtain
    // raw_predictions
    multinomial_deviance.get_init_raw_predictions(
        party, raw_predictions, sample_size * gbdt_model.class_num,
        training_data, training_labels);
    // init the encrypted labels, only the active party can init,
    // for multi-class classification, init one-hot labels for each class
    auto *encrypted_true_labels =
        new EncodedNumber[sample_size * gbdt_model.class_num];
    if (party.party_type == falcon::ACTIVE_PARTY) {
      // add the dummy mean prediction to the gbdt_model
      for (int i = 0; i < gbdt_model.class_num; i++) {
        gbdt_model.dummy_predictors.emplace_back(
            multinomial_deviance.dummy_predictions[i]);
      }
      double positive_class = 1.0;
      double negative_class = 0.0;
      for (int c = 0; c < gbdt_model.class_num; c++) {
        // compute one hot label vector
        std::vector<double> one_hot_labels;
        for (int i = 0; i < sample_size; i++) {
          if ((int)training_labels[i] == c) {
            one_hot_labels.push_back(positive_class);
          } else {
            one_hot_labels.push_back(negative_class);
          }
        }
        for (int i = 0; i < sample_size; i++) {
          int index = c * sample_size + i;
          encrypted_true_labels[index].set_double(phe_pub_key->n[0],
                                                  one_hot_labels[i]);
          djcs_t_aux_encrypt(phe_pub_key, party.phe_random,
                             encrypted_true_labels[index],
                             encrypted_true_labels[index]);
        }
      }
    }
    // active party broadcasts the encrypted true labels
    broadcast_encoded_number_array(party, encrypted_true_labels,
                                   sample_size * gbdt_model.class_num,
                                   ACTIVE_PARTY_ID);

    // iteratively train the regression trees, tree_size = n_estimator *
    // class_num
    log_info("tree_size = " + std::to_string(gbdt_model.tree_size));
    log_info("n_estimator = " + std::to_string(gbdt_model.n_estimator));
    log_info("class_num = " + std::to_string(gbdt_model.class_num));
    for (int tree_id = 0; tree_id < gbdt_model.n_estimator; tree_id++) {
      log_info("------------- build the " + std::to_string(tree_id) +
               "-th tree -------------");
      // step 2
      // compute the encrypted residual, i.e., negative gradient
      EncodedNumber *residuals =
          new EncodedNumber[sample_size * gbdt_model.class_num];
      multinomial_deviance.negative_gradient(
          party, encrypted_true_labels, raw_predictions, residuals,
          sample_size * gbdt_model.class_num);
      log_info("negative gradient computed finished");
      EncodedNumber *squared_residuals =
          new EncodedNumber[sample_size * gbdt_model.class_num];
      square_encrypted_residual(party, residuals, squared_residuals,
                                sample_size * gbdt_model.class_num,
                                PHE_FIXED_POINT_PRECISION);

      // build a tree for each class
      for (int c = 0; c < gbdt_model.class_num; c++) {
        log_info("------ build the " + std::to_string(tree_id) +
                 "-th tree ------ with class ------" + std::to_string(c));
        // this is the tree id in the gbdt_model.trees
        int read_tree_id = tree_id * gbdt_model.class_num + c;
        // flatten the residuals and squared_residuals into one vector for
        // calling the train_with_encrypted_labels in cart_tree.h
        EncodedNumber *flatten_residuals =
            new EncodedNumber[REGRESSION_TREE_CLASS_NUM * sample_size];
        for (int i = 0; i < sample_size; i++) {
          int real_sample_id = c * sample_size + i;
          flatten_residuals[i] = residuals[real_sample_id];
          flatten_residuals[sample_size + i] =
              squared_residuals[real_sample_id];
        }
        log_info("compute squared residuals finished");

        // step 3
        // init a tree builder and train the model with flatten_residuals
        // check gbdt task type, note that the gbdt type can be classification,
        // but all the trained decision trees are regression trees
        DecisionTreeParams classification_dt_params = dt_param;
        classification_dt_params.tree_type = "regression";
        classification_dt_params.class_num = REGRESSION_TREE_CLASS_NUM;
        tree_builders.emplace_back(classification_dt_params, training_data,
                                   testing_data, training_labels,
                                   testing_labels, training_accuracy,
                                   testing_accuracy);
        tree_builders[read_tree_id].train(party, flatten_residuals);
        log_info("tree builder = " + std::to_string(read_tree_id) +
                 " train finished");

        // step 4
        // after training the model, update the terminal regions,
        // note that we need to copy the read_tree_raw_predictions and
        // read_tree_encrypted_truth_labels for this real_tree_id from the
        // original raw_predictions and encrypted_truth_labels before update
        // terminal regions
        auto *real_tree_encrypted_truth_labels = new EncodedNumber[sample_size];
        auto *real_tree_raw_predictions = new EncodedNumber[sample_size];
        auto *real_tree_residuals = new EncodedNumber[sample_size];
        for (int i = 0; i < sample_size; i++) {
          int real_sample_id = c * sample_size + i;
          real_tree_encrypted_truth_labels[i] =
              encrypted_true_labels[real_sample_id];
          real_tree_raw_predictions[i] = raw_predictions[real_sample_id];
          real_tree_residuals[i] = residuals[real_sample_id];
        }
        multinomial_deviance.update_terminal_regions(
            party, tree_builders[read_tree_id],
            real_tree_encrypted_truth_labels, real_tree_residuals,
            real_tree_raw_predictions, sample_size, learning_rate, 0);
        // after update terminal regions, also update the raw_predictions
        // for the next iteration usage, the other two vectors can be discarded
        for (int i = 0; i < sample_size; i++) {
          int real_sample_id = c * sample_size + i;
          raw_predictions[real_sample_id] = real_tree_raw_predictions[i];
        }
        log_info("update terminal regions and raw predictions finished");
        // save the trained regression tree model to gbdt_model
        gbdt_model.gbdt_trees.emplace_back(tree_builders[read_tree_id].tree);
        delete[] flatten_residuals;
        delete[] real_tree_encrypted_truth_labels;
        delete[] real_tree_raw_predictions;
        delete[] real_tree_residuals;
      }
      delete[] residuals;
      delete[] squared_residuals;
    }
    delete[] raw_predictions;
    delete[] encrypted_true_labels;
  }

  // free retrieved public key
  djcs_t_free_public_key(phe_pub_key);
}

void GbdtBuilder::square_encrypted_residual(Party party,
                                            EncodedNumber *residuals,
                                            EncodedNumber *squared_residuals,
                                            int size, int phe_precision) {
  // given the encrypted residuals, need to compute the encrypted squared
  // labels, which is needed by the train_with_encrypted_labels api
  std::vector<int> public_values;
  std::vector<double> private_values;
  int class_num_for_regression = 1;
  public_values.push_back(size);
  public_values.push_back(class_num_for_regression);
  log_info("size = " + std::to_string(size));
  log_info("class_num = " + std::to_string(class_num_for_regression));
  // convert the encrypted residuals into secret shares
  // the structure is one-dimensional vector, [tree_1 * sample_size] ... [tree_n
  // * sample_size]
  std::vector<double> residuals_shares;
  ciphers_to_secret_shares(party, residuals, residuals_shares, size,
                           ACTIVE_PARTY_ID, phe_precision);
  // pack to private values for sending to mpc parties
  for (int i = 0; i < size; i++) {
    private_values.push_back(residuals_shares[i]);
  }

  falcon::SpdzTreeCompType comp_type = falcon::GBDT_SQUARE_LABEL;

  // communicate with spdz parties and receive results to compute labels
  // first send computation type, tree type, class num
  // then send private values
  std::promise<std::vector<double>> promise_values;
  std::future<std::vector<double>> future_values = promise_values.get_future();
  std::thread spdz_pruning_check_thread(
      spdz_tree_computation, party.party_num, party.party_id,
      party.executor_mpc_ports, party.host_names, public_values.size(),
      public_values, private_values.size(), private_values, comp_type,
      &promise_values);
  std::vector<double> squared_residuals_shares = future_values.get();
  spdz_pruning_check_thread.join();

  // convert the secret shares to ciphertext, which is encrypted square
  // residuals
  secret_shares_to_ciphers(party, squared_residuals, squared_residuals_shares,
                           size, ACTIVE_PARTY_ID, phe_precision);
}

void GbdtBuilder::eval(Party party, falcon::DatasetType eval_type,
                       const string &report_save_path) {
  std::string dataset_str =
      (eval_type == falcon::TRAIN ? "training dataset" : "testing dataset");
  log_info("************* Evaluation on " + dataset_str +
           " Start *************");
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  // init test data
  int dataset_size =
      (eval_type == falcon::TRAIN) ? training_data.size() : testing_data.size();
  std::vector<std::vector<double>> cur_test_dataset =
      (eval_type == falcon::TRAIN) ? training_data : testing_data;
  std::vector<double> cur_test_dataset_labels =
      (eval_type == falcon::TRAIN) ? training_labels : testing_labels;

  // compute predictions
  // now the predicted labels are computed by mpc, thus it is already the final
  // label
  int prediction_result_size = dataset_size;
  if (gbdt_model.tree_type == falcon::CLASSIFICATION &&
      gbdt_model.class_num > 2) {
    prediction_result_size = dataset_size * gbdt_model.class_num;
  }
  auto *predicted_labels = new EncodedNumber[prediction_result_size];
  gbdt_model.predict(party, cur_test_dataset, dataset_size, predicted_labels);

  // step 3: active party aggregates and call collaborative decryption
  auto *decrypted_labels = new EncodedNumber[prediction_result_size];
  collaborative_decrypt(party, predicted_labels, decrypted_labels,
                        prediction_result_size, ACTIVE_PARTY_ID);

  // calculate accuracy by the active party
  std::vector<double> predictions;
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // decode decrypted predicted labels
    for (int i = 0; i < prediction_result_size; i++) {
      double x;
      decrypted_labels[i].decode(x);
      predictions.push_back(x);
    }

    // compute accuracy
    if (gbdt_model.tree_type == falcon::CLASSIFICATION) {
      int correct_num = 0;
      if (gbdt_model.class_num == 2) {
        // binary classification
        for (int i = 0; i < prediction_result_size; i++) {
          log_info("before predictions[" + std::to_string(i) +
                   "] = " + std::to_string(predictions[i]));
          predictions[i] = (predictions[i] > LOGREG_THRES) ? CERTAIN_PROBABILITY
                                                           : ZERO_PROBABILITY;
          log_info("after predictions[" + std::to_string(i) +
                   "] = " + std::to_string(predictions[i]));
          if (predictions[i] == cur_test_dataset_labels[i]) {
            correct_num += 1;
          }
        }
      }
      if (gbdt_model.class_num > 2) {
        // multi-class classification
        for (int i = 0; i < dataset_size; i++) {
          std::vector<double> predictions_per_sample;
          for (int c = 0; c < gbdt_model.class_num; c++) {
            log_info("prediction " + std::to_string(i) + ": class num " +
                     std::to_string(c) + " = " +
                     std::to_string(predictions[c * dataset_size + i]));
            predictions_per_sample.push_back(predictions[c * dataset_size + i]);
          }
          // find argmax class label
          double prediction_decision = argmax(predictions_per_sample);
          if (prediction_decision == cur_test_dataset_labels[i]) {
            correct_num += 1;
          }
        }
      }
      if (eval_type == falcon::TRAIN) {
        training_accuracy = (double)correct_num / dataset_size;
        log_info("[GbdtBuilder.eval] Dataset size = " +
                 std::to_string(dataset_size) +
                 ", correct predicted num = " + std::to_string(correct_num) +
                 ", training accuracy = " + std::to_string(training_accuracy));
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy = (double)correct_num / dataset_size;
        log_info("[GbdtBuilder.eval] Dataset size = " +
                 std::to_string(dataset_size) +
                 ", correct predicted num = " + std::to_string(correct_num) +
                 ", testing accuracy = " + std::to_string(testing_accuracy));
      }
    } else {
      if (eval_type == falcon::TRAIN) {
        training_accuracy =
            mean_squared_error(predictions, cur_test_dataset_labels);
        log_info("[GbdtBuilder.eval] Training accuracy = " +
                 std::to_string(training_accuracy));
      }
      if (eval_type == falcon::TEST) {
        testing_accuracy =
            mean_squared_error(predictions, cur_test_dataset_labels);
        log_info("[GbdtBuilder.eval] Testing accuracy = " +
                 std::to_string(testing_accuracy));
      }
    }
  }

  // free memory
  delete[] predicted_labels;
  delete[] decrypted_labels;

  struct timespec finish;
  clock_gettime(CLOCK_MONOTONIC, &finish);
  double consumed_time = (double)(finish.tv_sec - start.tv_sec);
  consumed_time += (double)(finish.tv_nsec - start.tv_nsec) / 1000000000.0;
  log_info("Evaluation time = " + std::to_string(consumed_time));
  log_info("************* Evaluation on " + dataset_str +
           " Finished *************");
}
