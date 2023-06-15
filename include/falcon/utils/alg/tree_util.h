//
// Created by root on 3/11/22.
//

#ifndef FALCON_INCLUDE_FALCON_UTILS_ALG_TREE_UTIL_H_
#define FALCON_INCLUDE_FALCON_UTILS_ALG_TREE_UTIL_H_

#include "falcon/common.h"
#include <falcon/operator/phe/djcs_t_aux.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <falcon/party/party.h>
#include <vector>

/**
 * compute the initial impurity for the root tree node
 * @param labels: the label vector
 * @param tree_type: whether classification or regression
 * @param class_num: the number of classes
 * @return
 */
double root_impurity(const std::vector<double> &labels,
                     falcon::TreeType tree_type, int class_num);

/**
 * TODO: this function is only for regression tree root impurity under lime
 * computation the label and label square vector are encrypted, as well as the
 * sample weights
 *
 * @param party
 * @param enc_root_impurity
 * @param use_encrypted_labels
 * @param weighted_encrypted_true_labels
 * @param size
 * @param class_num
 * @param use_sample_weights
 * @param sss_sample_weights
 * @return
 */
double lime_reg_tree_root_impurity(
    Party &party, EncodedNumber &enc_root_impurity, bool use_encrypted_labels,
    EncodedNumber *weighted_encrypted_true_labels, int size, int class_num,
    bool use_sample_weights, const std::vector<double> &sss_sample_weights);

std::vector<double> rf_pred2prob(int class_num,
                                 const std::vector<double> &pred);

#endif // FALCON_INCLUDE_FALCON_UTILS_ALG_TREE_UTIL_H_
