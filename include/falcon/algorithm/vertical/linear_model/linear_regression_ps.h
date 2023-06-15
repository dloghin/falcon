//
// Created by root on 11/17/21.
//

#ifndef FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_PS_H_
#define FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_PS_H_

#include "falcon/distributed/parameter_server_base.h"
#include <falcon/algorithm/vertical/linear_model/linear_model_base.h>
#include <falcon/algorithm/vertical/linear_model/linear_model_ps.h>
#include <falcon/algorithm/vertical/linear_model/linear_regression_builder.h>
#include <falcon/algorithm/vertical/linear_model/logistic_regression_model.h>
#include <falcon/common.h>
#include <falcon/model/model_io.h>
#include <falcon/party/party.h>
#include <falcon/utils/pb_converter/common_converter.h>
#include <falcon/utils/pb_converter/lr_converter.h>

using namespace std;

class LinearRegParameterServer : public LinearParameterServer {
public:
  // LR model builder
  LinearRegressionBuilder alg_builder;

public:
  /**
   * constructor
   *
   * @param m_alg_builder: created algorithm builder
   * @param m_party: the participating party
   * @param ps_network_config_pb_str: the network config between ps and workers
   */
  LinearRegParameterServer(const LinearRegressionBuilder &m_alg_builder,
                           const Party &m_party,
                           const std::string &ps_network_config_pb_str);

  /**
   * constructor
   *
   * @param m_party: the participating party
   * @param ps_network_config_pb_str: the network config between ps and workers
   */
  LinearRegParameterServer(const Party &m_party,
                           const std::string &ps_network_config_pb_str);

  /**
   * copy constructor
   * @param linear regression parameter server
   */
  LinearRegParameterServer(const LinearRegParameterServer &obj);

  /**
   * default destructor
   */
  ~LinearRegParameterServer();

public:
  /**
   * distributed training process, partition data, collect result, update
   * weights
   */
  void distributed_train() override;

  /**
   * distributed evaluation, partition data, collect result, compute evaluation
   * matrix
   *
   * @param eval_type: train data or test data
   * @param report_save_path: the path to save evaluation matrix
   */
  void distributed_eval(falcon::DatasetType eval_type,
                        const std::string &report_save_path) override;

  /**
   * save the trained model
   *
   * @param model_save_file: vector of index
   */
  void save_model(const std::string &model_save_file) override;
};

#endif // FALCON_INCLUDE_FALCON_ALGORITHM_VERTICAL_LINEAR_MODEL_LINEAR_REGRESSION_PS_H_
