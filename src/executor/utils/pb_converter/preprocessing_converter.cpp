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
// Created by naili on 21/6/22.
//


#include "falcon/utils/pb_converter/preprocessing_converter.h"
#include <glog/logging.h>
#include <falcon/utils/logger/logger.h>
#include "../../include/message/feat_sel.pb.h"


void serialize_feat_sel_params(const FeatSelParams&_feat_sel_params, std::string& output_message) {
  com::nus::dbsytem::falcon::v0::FeatSelParams pb_feat_sel_params;
  pb_feat_sel_params.set_num_samples(_feat_sel_params.num_samples);
  pb_feat_sel_params.set_feature_selection(_feat_sel_params.feature_selection);
  pb_feat_sel_params.set_selected_features_file(_feat_sel_params.selected_features_file);

  pb_feat_sel_params.SerializeToString(&output_message);
}

void deserialize_feat_sel_params(FeatSelParams& feat_sel_params, const std::string& input_message) {
  com::nus::dbsytem::falcon::v0::FeatSelParams pb_feat_sel_params;
  if (!pb_feat_sel_params.ParseFromString(input_message)) {
    log_error("Deserialize feature selection params message failed.");
    exit(EXIT_FAILURE);
  }
  feat_sel_params.num_samples = pb_feat_sel_params.num_samples();
  feat_sel_params.feature_selection = pb_feat_sel_params.feature_selection();
  feat_sel_params.selected_features_file = pb_feat_sel_params.selected_features_file();
}