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
// Created by root on 11/13/21.
//

#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/common.h>
#include <falcon/operator/conversion/op_conv.h>
#include <falcon/party/info_exchange.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/pb_converter/common_converter.h>

#include <cmath>
#include <glog/logging.h>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stack>

LinearModel::LinearModel() = default;

LinearModel::LinearModel(int m_weight_size) {
  weight_size = m_weight_size;
  local_weights = new EncodedNumber[weight_size];
}

LinearModel::~LinearModel() { delete[] local_weights; }

LinearModel::LinearModel(const LinearModel &linear_model) {
  weight_size = linear_model.weight_size;
  local_weights = new EncodedNumber[weight_size];
  for (int i = 0; i < weight_size; i++) {
    local_weights[i] = linear_model.local_weights[i];
  }
  party_weight_sizes = linear_model.party_weight_sizes;
}

LinearModel &LinearModel::operator=(const LinearModel &linear_model) {
  weight_size = linear_model.weight_size;
  local_weights = new EncodedNumber[weight_size];
  for (int i = 0; i < weight_size; i++) {
    local_weights[i] = linear_model.local_weights[i];
  }
  party_weight_sizes = linear_model.party_weight_sizes;
}

void LinearModel::compute_batch_phe_aggregation(
    const Party &party, int cur_batch_size,
    EncodedNumber **encoded_batch_samples, int precision,
    EncodedNumber *encrypted_batch_aggregation) const {
  // retrieve phe pub key and phe random
  djcs_t_public_key *phe_pub_key = djcs_t_init_public_key();
  party.getter_phe_pub_key(phe_pub_key);
  // each party compute local homomorphic aggregation
  auto *local_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
  djcs_t_aux_vec_mat_ep_mult(
      phe_pub_key, party.phe_random, local_batch_phe_aggregation, local_weights,
      encoded_batch_samples, cur_batch_size, weight_size);
  // every party sends the local aggregation to the active party
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // copy self local aggregation results
    for (int i = 0; i < cur_batch_size; i++) {
      // each element (i) in local_batch_phe_aggregation is [W].Xi,
      // Xi is a part of feature vector in example i
      encrypted_batch_aggregation[i] = local_batch_phe_aggregation[i];
    }

    // 1. active party receive serialized string from other parties
    // deserialize and sum to batch_phe_aggregation
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        auto *recv_batch_phe_aggregation = new EncodedNumber[cur_batch_size];
        std::string recv_local_aggregation_str;
        party.recv_long_message(id, recv_local_aggregation_str);
        deserialize_encoded_number_array(recv_batch_phe_aggregation,
                                         cur_batch_size,
                                         recv_local_aggregation_str);
        // homomorphic addition of the received aggregations
        // After addition, each element (i) in batch_phe_aggregation is
        // [W1].Xi1+[W2].Xi2+...+[Wn].Xin Xin is example i's n-th feature,
        // W1 is first element in weigh vector
        for (int i = 0; i < cur_batch_size; i++) {
          djcs_t_aux_ee_add(phe_pub_key, encrypted_batch_aggregation[i],
                            encrypted_batch_aggregation[i],
                            recv_batch_phe_aggregation[i]);
        }
        delete[] recv_batch_phe_aggregation;
      }
    }
    // 2. active party serialize and send the aggregated
    // batch_phe_aggregation to other parties
    std::string global_aggregation_str;
    serialize_encoded_number_array(encrypted_batch_aggregation, cur_batch_size,
                                   global_aggregation_str);
    for (int id = 0; id < party.party_num; id++) {
      if (id != party.party_id) {
        party.send_long_message(id, global_aggregation_str);
      }
    }
  } else {
    // if it's passive party, it will not do the aggregation,
    // 1. it just serializes local batch aggregation and send to active party
    std::string local_aggregation_str;
    serialize_encoded_number_array(local_batch_phe_aggregation, cur_batch_size,
                                   local_aggregation_str);
    party.send_long_message(ACTIVE_PARTY_ID, local_aggregation_str);
    // 2. passive party receive the global batch aggregation
    // from the active party
    std::string recv_global_aggregation_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_aggregation_str);
    deserialize_encoded_number_array(encrypted_batch_aggregation,
                                     cur_batch_size,
                                     recv_global_aggregation_str);
  }

  djcs_t_free_public_key(phe_pub_key);
  delete[] local_batch_phe_aggregation;
}

void LinearModel::sync_up_weight_sizes(const Party &party) {
  std::vector<int> sync_arr = sync_up_int_arr(party, weight_size);
  party_weight_sizes = sync_arr;
}

void LinearModel::truncate_weights_precision(const Party &party,
                                             int dest_precision) {
  // each party need to truncate the weight precision
  // let active party aggregate the encrypted gradients and broadcast, and
  // truncate
  log_info("abs(local_weight[0].getter_exponent) = " +
           std::to_string(abs(local_weights[0].getter_exponent())));
  int global_weight_size =
      std::accumulate(party_weight_sizes.begin(), party_weight_sizes.end(), 0);
  auto *global_encrypted_weights = new EncodedNumber[global_weight_size];
  if (party.party_type == falcon::ACTIVE_PARTY) {
    // first, pack its own encrypted gradients
    int idx = 0;
    for (int j = 0; j < weight_size; j++) {
      global_encrypted_weights[idx] = local_weights[j];
      idx += 1;
    }
    // second, receive each party's encrypted gradients
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        std::string recv_enc_weights_i_str;
        party.recv_long_message(i, recv_enc_weights_i_str);
        auto *recv_enc_weights_i = new EncodedNumber[party_weight_sizes[i]];
        deserialize_encoded_number_array(
            recv_enc_weights_i, party_weight_sizes[i], recv_enc_weights_i_str);
        for (int j = 0; j < party_weight_sizes[i]; j++) {
          global_encrypted_weights[idx] = recv_enc_weights_i[j];
          idx += 1;
        }
        delete[] recv_enc_weights_i;
      }
    }
    // third, broadcast this array, such that parties can truncate
    std::string global_enc_weights_str;
    serialize_encoded_number_array(global_encrypted_weights, global_weight_size,
                                   global_enc_weights_str);
    for (int i = 0; i < party.party_num; i++) {
      if (i != party.party_id) {
        party.send_long_message(i, global_enc_weights_str);
      }
    }
  } else {
    // first, send encrypted gradients to the active party
    std::string enc_weights_str;
    serialize_encoded_number_array(local_weights, weight_size, enc_weights_str);
    party.send_long_message(ACTIVE_PARTY_ID, enc_weights_str);
    // second, receive global encrypted gradients array
    std::string recv_global_enc_weights_str;
    party.recv_long_message(ACTIVE_PARTY_ID, recv_global_enc_weights_str);
    deserialize_encoded_number_array(global_encrypted_weights,
                                     global_weight_size,
                                     recv_global_enc_weights_str);
  }
  truncate_ciphers_precision(party, global_encrypted_weights,
                             global_weight_size, ACTIVE_PARTY_ID,
                             dest_precision);
  // find the corresponding encrypted gradients needed
  int start_idx_in_global = 0;
  for (int i = 0; i < party.party_num; i++) {
    if (i < party.party_id) {
      start_idx_in_global += party_weight_sizes[i];
    } else {
      break;
    }
  }
  for (int j = 0; j < weight_size; j++) {
    local_weights[j] = global_encrypted_weights[start_idx_in_global];
    start_idx_in_global += 1;
  }

  delete[] global_encrypted_weights;
}

std::vector<double> LinearModel::display_weights(const Party &party) {
  std::vector<double> local_model_weights;
  log_info("display local weights");
  for (int i = 0; i < party.party_num; i++) {
    log_info("party " + std::to_string(i) +
             "'s weight size = " + std::to_string(party_weight_sizes[i]));
  }
  for (int i = 0; i < party.party_num; i++) {
    log_info("decrypt party " + std::to_string(i) + "'s weights");
    int party_i_weight_size = party_weight_sizes[i];
    auto *party_i_local_weights = new EncodedNumber[party_i_weight_size];
    auto *party_i_decrypted_weights = new EncodedNumber[party_i_weight_size];
    // broadcast local weights and call decrypt
    if (i == party.party_id) {
      log_info("I am party " + std::to_string(i) + ", i am packaging weights");
      for (int j = 0; j < party_i_weight_size; j++) {
        party_i_local_weights[j] = local_weights[j];
      }
    }
    broadcast_encoded_number_array(party, party_i_local_weights,
                                   party_i_weight_size, i);
    collaborative_decrypt(party, party_i_local_weights,
                          party_i_decrypted_weights, party_i_weight_size, i);
    if (i == party.party_id) {
      for (int j = 0; j < party_i_weight_size; j++) {
        double weight;
        party_i_decrypted_weights[j].decode(weight);
        log_info("local weight[" + std::to_string(j) +
                 "] = " + std::to_string(weight));
        local_model_weights.push_back(weight);
      }
    }
    delete[] party_i_local_weights;
    delete[] party_i_decrypted_weights;
  }
  return local_model_weights;
}
