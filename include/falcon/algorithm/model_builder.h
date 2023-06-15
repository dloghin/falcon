//
// Created by wuyuncheng on 14/11/20.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_MODEL_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_MODEL_H_

#include <falcon/distributed/worker.h>
#include <falcon/party/party.h>
#include <utility>
#include <vector>

class ModelBuilder {
protected:
  // training data (without label column)
  std::vector<std::vector<double>> training_data;
  // testing data (without label column)
  std::vector<std::vector<double>> testing_data;
  // labels of training samples
  std::vector<double> training_labels;
  // labels of testing samples
  std::vector<double> testing_labels;
  // training accuracy
  double training_accuracy;
  // testing accuracy
  double testing_accuracy;

public:
  /**
   * default constructor
   */
  ModelBuilder();

  ModelBuilder(const ModelBuilder &builder);

  /**
   * model constructor
   *
   * @param m_training_data
   * @param m_testing_data
   * @param m_training_labels
   * @param m_testing_labels
   * @param m_training_accuracy
   * @param m_testing_accuracy
   */
  ModelBuilder(std::vector<std::vector<double>> m_training_data,
               std::vector<std::vector<double>> m_testing_data,
               std::vector<double> m_training_labels,
               std::vector<double> m_testing_labels,
               double m_training_accuracy = 0.0,
               double m_testing_accuracy = 0.0);

  /**
   * destructor
   */
  ~ModelBuilder();

  /**
   * build the model
   * @param party
   */
  virtual void train(Party party) = 0;

  /**
   * distributed train method
   *
   * @param party
   * @param worker
   */
  virtual void distributed_train(const Party &party, const Worker &worker) = 0;

  /**
   * evaluate the accuracy on a dataset
   * @param party
   * @param eval_type
   * @param report_save_path
   */
  virtual void eval(Party party, falcon::DatasetType eval_type,
                    const std::string &report_save_path = std::string()) = 0;

  /** set training data */
  void setter_training_data(std::vector<std::vector<double>> s_training_data) {
    training_data = std::move(s_training_data);
  }

  /** set testing data */
  void setter_testing_data(std::vector<std::vector<double>> s_testing_data) {
    testing_data = std::move(s_testing_data);
  }

  /** set training labels */
  void setter_training_labels(std::vector<double> s_training_labels) {
    training_labels = std::move(s_training_labels);
  }

  /** set testing labels */
  void setter_testing_labels(std::vector<double> s_testing_labels) {
    testing_labels = std::move(s_testing_labels);
  }

  /** set training accuracy */
  void setter_training_accuracy(double s_training_accuracy) {
    training_accuracy = s_training_accuracy;
  }

  /** set testing accuracy */
  void setter_testing_accuracy(double s_testing_accuracy) {
    testing_accuracy = s_testing_accuracy;
  }

  /** get training data */
  std::vector<std::vector<double>> getter_training_data() {
    return training_data;
  }

  /** get testing data */
  std::vector<std::vector<double>> getter_testing_data() {
    return testing_data;
  }

  /** get training labels */
  std::vector<double> getter_training_labels() { return training_labels; }

  /** get testing labels */
  std::vector<double> getter_testing_labels() { return testing_labels; }

  /** get training accuracy */
  double getter_training_accuracy() { return training_accuracy; }

  /** get testing accuracy */
  double getter_testing_accuracy() { return testing_accuracy; }
};

#endif // FALCON_SRC_EXECUTOR_ALGORITHM_MODEL_H_
