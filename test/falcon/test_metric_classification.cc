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

#include <falcon/utils/metric/classification.h>
#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <vector>

TEST(Classification_metrics, test_TP) {
  std::vector<int> predictions = {1, 0, 1, 0, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);
  EXPECT_EQ(ClfMetrics.TP, 3);
}

TEST(Classification_metrics, test_FP) {
  std::vector<int> predictions = {1, 0, 1, 0, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);
  EXPECT_EQ(ClfMetrics.FP, 0);
}

TEST(Classification_metrics, test_FN) {
  std::vector<int> predictions = {1, 1, 1, 0};
  std::vector<double> labels = {0, 1, 0, 1};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);
  EXPECT_EQ(ClfMetrics.FN, 1);
}

TEST(Classification_metrics, test_TN) {
  std::vector<int> predictions = {1, 1, 0, 0};
  std::vector<double> labels = {0, 1, 0, 1};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);
  EXPECT_EQ(ClfMetrics.TN, 1);
}

TEST(Classification_metrics, test_total_num) {
  std::vector<int> predictions = {1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
  std::vector<double> labels = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);

  double all_4_cells =
      ClfMetrics.TP + ClfMetrics.FP + ClfMetrics.FN + ClfMetrics.TN;
  EXPECT_EQ(all_4_cells, predictions.size());
}

TEST(Classification_metrics, test_regular_acc) {
  // Given a sample of 13 pictures, 8 of cats and 5 of dogs
  // classifier makes 8 accurate predictions and misses 5
  // 3 cats wrongly predicted as dogs (first 3 predictions)
  // and 2 dogs wrongly predicted as cats (last 2 predictions).
  std::vector<int> predictions = {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);

  EXPECT_EQ(ClfMetrics.regular_accuracy, (double)8 / 13);
}

TEST(Classification_metrics, test_sensitivity) {
  // Given a sample of 13 pictures, 8 of cats and 5 of dogs
  // classifier makes 8 accurate predictions and misses 5
  // 3 cats wrongly predicted as dogs (first 3 predictions)
  // and 2 dogs wrongly predicted as cats (last 2 predictions).
  std::vector<int> predictions = {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);

  EXPECT_EQ(ClfMetrics.sensitivity, (double)5 / 8);
}

TEST(Classification_metrics, test_specificity) {
  // Given a sample of 13 pictures, 8 of cats and 5 of dogs
  // classifier makes 8 accurate predictions and misses 5
  // 3 cats wrongly predicted as dogs (first 3 predictions)
  // and 2 dogs wrongly predicted as cats (last 2 predictions).
  std::vector<int> predictions = {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);

  EXPECT_EQ(ClfMetrics.specificity, (double)3 / 5);
}

TEST(Classification_metrics, test_balanced_acc) {
  // Given a sample of 13 pictures, 8 of cats and 5 of dogs
  // classifier makes 8 accurate predictions and misses 5
  // 3 cats wrongly predicted as dogs (first 3 predictions)
  // and 2 dogs wrongly predicted as cats (last 2 predictions).
  std::vector<int> predictions = {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);

  EXPECT_EQ(ClfMetrics.balanced_accuracy,
            (ClfMetrics.sensitivity + ClfMetrics.specificity) / 2);

  EXPECT_FLOAT_EQ(ClfMetrics.balanced_accuracy, 0.6125);
}

TEST(Classification_metrics, test_predictive_values) {
  // Given a sample of 13 pictures, 8 of cats and 5 of dogs
  // classifier makes 8 accurate predictions and misses 5
  // 3 cats wrongly predicted as dogs (first 3 predictions)
  // and 2 dogs wrongly predicted as cats (last 2 predictions).
  std::vector<int> predictions = {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);

  EXPECT_EQ(ClfMetrics.precision, (double)5 / 7);
  EXPECT_EQ(ClfMetrics.NPV, (double)3 / 6);
}

TEST(Classification_metrics, test_f1) {
  // Given a sample of 13 pictures, 8 of cats and 5 of dogs
  // classifier makes 8 accurate predictions and misses 5
  // 3 cats wrongly predicted as dogs (first 3 predictions)
  // and 2 dogs wrongly predicted as cats (last 2 predictions).
  std::vector<int> predictions = {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1};
  std::vector<double> labels = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

  ClassificationMetrics ClfMetrics;
  ClfMetrics.compute_metrics(predictions, labels);

  // F1 = 2TP / (2TP + FP + FN)
  // F1 = 2 * (precision * recall) / (precision + recall)
  EXPECT_EQ(ClfMetrics.F1, (double)10 / 15);
  EXPECT_EQ(ClfMetrics.F1,
            ((double)2 * ClfMetrics.precision * ClfMetrics.sensitivity /
             (ClfMetrics.precision + ClfMetrics.sensitivity)));
}
