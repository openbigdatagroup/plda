// Copyright 2008 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cmd_flags.h"

#include <iostream>
#include <sstream>

namespace learning_lda {

LDACmdLineFlags::LDACmdLineFlags() {
  // Assign all flags invalid values, so CheckValidity will enforce
  // users provide valid values.
  num_topics_ = 0;
  alpha_ = -1;
  beta_ = -1;
  training_data_file_ = "";
  inference_data_file_ = "";
  inference_result_file_ = "";
  model_file_ = "";
  burn_in_iterations_ = -1;
  total_iterations_ = -1;
  compute_likelihood_ = "false";
}

void LDACmdLineFlags::ParseCmdFlags(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (0 == strcmp(argv[i], "--num_topics")) {
      std::istringstream(argv[i+1]) >> num_topics_;
      ++i;
    } else if (0 == strcmp(argv[i], "--alpha")) {
      std::istringstream(argv[i+1]) >> alpha_;
      ++i;
    } else if (0 == strcmp(argv[i], "--beta")) {
      std::istringstream(argv[i+1]) >> beta_;
      ++i;
    } else if (0 == strcmp(argv[i], "--training_data_file")) {
      training_data_file_ = argv[i+1];
      ++i;
    } else if (0 == strcmp(argv[i], "--model_file")) {
      model_file_ = argv[i+1];
      ++i;
    } else if (0 == strcmp(argv[i], "--inference_data_file")) {
      inference_data_file_ = argv[i+1];
      ++i;
    } else if (0 == strcmp(argv[i], "--inference_result_file")) {
      inference_result_file_ = argv[i+1];
      ++i;
    } else if (0 == strcmp(argv[i], "--burn_in_iterations")) {
      std::istringstream(argv[i+1]) >> burn_in_iterations_;
      ++i;
    } else if (0 == strcmp(argv[i], "--total_iterations")) {
      std::istringstream(argv[i+1]) >> total_iterations_;
      ++i;
    } else if (0 == strcmp(argv[i], "--compute_likelihood")) {
      compute_likelihood_ = argv[i+1];
      ++i;
    }

  }
}

bool LDACmdLineFlags::CheckTrainingValidity() {
  bool ret = true;
  if (num_topics_ <= 1) {
    std::cerr << "num_topics must >= 2.\n";
    ret = false;
  }
  if (alpha_ <= 0) {
    std::cerr << "alpha must > 0.\n";
    ret = false;
  }
  if (beta_ <= 0) {
    std::cerr << "beta must > 0.\n";
    ret = false;
  }
  if (training_data_file_.empty()) {
    std::cerr << "Invalid training_data_file.\n";
    ret = false;
  }
  if (model_file_.empty()) {
    std::cerr << "Invalid model_file.\n";
    ret = false;
  }
  if (burn_in_iterations_ < 0) {
    std::cerr << "burn_in_iterations must >= 0.\n";
    ret = false;
  }
  if (total_iterations_ <= burn_in_iterations_) {
    std::cerr << "total_iterations must > burn_in_iterations.\n";
    ret = false;
  }
  return ret;
}

bool LDACmdLineFlags::CheckParallelTrainingValidity() {
  bool ret = true;
  if (num_topics_ <= 1) {
    std::cerr << "num_topics must >= 2.\n";
    ret = false;
  }
  if (alpha_ <= 0) {
    std::cerr << "alpha must > 0.\n";
    ret = false;
  }
  if (beta_ <= 0) {
    std::cerr << "beta must > 0.\n";
    ret = false;
  }
  if (training_data_file_.empty()) {
    std::cerr << "Invalid training_data_file.\n";
    ret = false;
  }
  if (model_file_.empty()) {
    std::cerr << "Invalid model_file.\n";
    ret = false;
  }
  if (total_iterations_ <= 0) {
    std::cerr << "total_iterations must > 0.\n";
    ret = false;
  }
  if (compute_likelihood_ != "true" && compute_likelihood_ != "false") {
    std::cerr << "compute_likelihood must be true or false.\n";
    ret = false;
  }
  return ret;
}
bool LDACmdLineFlags::CheckInferringValidity() {
  bool ret = true;
  if (alpha_ <= 0) {
    std::cerr << "alpha must > 0.\n";
    ret = false;
  }
  if (beta_ <= 0) {
    std::cerr << "beta must > 0.\n";
    ret = false;
  }
  if (inference_data_file_.empty()) {
    std::cerr << "Invalid inference_data_file.\n";
    ret = false;
  }
  if (inference_result_file_.empty()) {
    std::cerr << "Invalid inference_result_file.\n";
    ret = false;
  }
  if (model_file_.empty()) {
    std::cerr << "Invalid model_file.\n";
    ret = false;
  }
  if (burn_in_iterations_ < 0) {
    std::cerr << "burn_in_iterations must >= 0.\n";
    ret = false;
  }
  if (total_iterations_ <= burn_in_iterations_) {
    std::cerr << "total_iterations must > burn_in_iterations.\n";
    ret = false;
  }
  return ret;
}

}  // namespace learning_lda
