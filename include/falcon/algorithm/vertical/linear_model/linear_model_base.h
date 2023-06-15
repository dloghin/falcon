//
// Created by root on 11/13/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BASE_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BASE_H_

#include "falcon/distributed/worker.h"
#include <falcon/common.h>
#include <falcon/party/party.h>

#include <future>
#include <thread>

class LinearModel {
public:
  // number of weights in the model
  int weight_size{};
  // model weights vector, encrypted values during training, size equals to
  // weight_size
  EncodedNumber *local_weights{};
  // parties' weight size vector
  std::vector<int> party_weight_sizes;

public:
  LinearModel();
  explicit LinearModel(int m_weight_size);
  ~LinearModel();

  /**
   * copy constructor
   * @param linear_model
   */
  LinearModel(const LinearModel &linear_model);

  /**
   * assignment constructor
   * @param log_reg_model
   * @return
   */
  LinearModel &operator=(const LinearModel &linear_model);

  /**
   * compute phe aggregation for a batch of samples
   *
   * @param party: initialized party object
   * @param batch_indexes: selected batch indexes
   * @param dataset_type: denote the dataset type
   * @param precision: the fixed point precision of encoded plaintext samples
   * @param batch_aggregation: returned phe aggregation for the batch
   */
  void compute_batch_phe_aggregation(
      const Party &party, int cur_batch_size,
      EncodedNumber **encoded_batch_samples, int precision,
      EncodedNumber *encrypted_batch_aggregation) const;

  /**
   * sync up the weight size vector
   *
   * @param party: initialized party object
   */
  void sync_up_weight_sizes(const Party &party);

  /**
   * This function truncates the parties' weights precision
   *
   * @param party: initialized party object
   * @param dest_precision: the destination precision
   */
  void truncate_weights_precision(const Party &party, int dest_precision);

  /**
   * print weights during training to view changes
   *
   * @param party: initialized party object
   */
  std::vector<double> display_weights(const Party &party);
};

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_MODEL_BASE_H_
