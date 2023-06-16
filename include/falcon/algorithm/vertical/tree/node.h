//
// Created by wuyuncheng on 12/5/21.
//

#ifndef FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_
#define FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_

#include <falcon/common.h>
#include <falcon/operator/phe/fixed_point_encoder.h>
#include <vector>

class Node {
public:
  // node type, default is internal node
  falcon::TreeNodeType node_type;
  // the depth of the current node, root node is 0, -1: not decided
  int depth;
  // if the node belongs to the party itself, 0: no, 1: yes, -1: not decided
  int is_self_feature;
  // the party that owns the selected feature on this node, -1: not decided
  int best_party_id;
  // the feature on this node, -1: not self feature, 0 -- d_i: self feature id
  int best_feature_id;
  // the split of the feature on this node, -1: not decided
  int best_split_id;
  // the split threshold if it is its own feature
  double split_threshold;
  // the number of samples where the element in sample_iv is [1]
  int node_sample_num;
  // the number of samples for each class on the node
  std::vector<int> node_sample_distribution;
  // node impurity, Gini index for classification, variance for regression
  EncodedNumber impurity;
  // if is_leaf is true, a label is assigned
  EncodedNumber label;
  // left branch id of the current node, if not a leaf node, -1: not decided
  int left_child;
  // right branch id of the current node, if not a leaf node, -1: not decided
  int right_child;

public:
  Node();
  ~Node();

  /**
   * copy constructor
   *
   * @param node
   */
  Node(const Node &node);

  /**
   * assignment constructor
   *
   * @param node
   * @return
   */
  Node &operator=(const Node &node);

  /**
   * print node information
   */
  void print_node();
};

#endif // FALCON_SRC_EXECUTOR_ALGORITHM_VERTICAL_TREE_NODE_H_
