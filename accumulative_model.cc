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

#include "accumulative_model.h"

#include <algorithm>
#include <functional>
#include <map>
#include <numeric>
#include <string>

namespace learning_lda {

LDAAccumulativeModel::LDAAccumulativeModel(int num_topics) {
  CHECK_LT(1, num_topics);
  global_distribution_.resize(num_topics, 0);
  zero_distribution_.resize(num_topics, 0);
}

// Accumulate a model into accumulative_topic_distributions_ and
// accumulative_global_distributions_.
void LDAAccumulativeModel::AccumulateModel(const LDAModel& source_model) {
  CHECK_EQ(num_topics(), source_model.num_topics());
  for (LDAModel::Iterator iter(&source_model); !iter.Done(); iter.Next()) {
    const TopicCountDistribution& source_dist = iter.Distribution();
    TopicProbDistribution* dest_dist = NULL;

    map<string, TopicProbDistribution>::iterator target_distribution =
        topic_distributions_.find(iter.Word());

    if (target_distribution == topic_distributions_.end()) {
      topic_distributions_[iter.Word()].resize(num_topics());
      dest_dist = &(topic_distributions_[iter.Word()]);
    } else {
      dest_dist = &(target_distribution->second);
    }

    CHECK_EQ(num_topics(), source_dist.size());
    for (int k = 0; k < num_topics(); ++k) {
      (*dest_dist)[k] += static_cast<double>(source_dist[k]);
    }
  }

  for (int k = 0; k < num_topics(); ++k) {
    global_distribution_[k] +=
        static_cast<double>(source_model.GetGlobalTopicDistribution()[k]);
  }
}

void LDAAccumulativeModel::AverageModel(int num_accumulations) {
  size_t vocab_size = topic_distributions_.size();

  for (map<string, TopicProbDistribution>::iterator iter =
           topic_distributions_.begin();
       iter != topic_distributions_.end();
       ++iter) {
    TopicProbDistribution& dist = iter->second;
    for (int k = 0; k < num_topics(); ++k) {
      dist[k] /= num_accumulations;
    }
  }
  for (int k = 0; k < num_topics(); ++k) {
    global_distribution_[k] /= num_accumulations;
  }
}

const TopicProbDistribution& LDAAccumulativeModel::GetWordTopicDistribution(
    const string& word) const {
  map<string, TopicProbDistribution>::const_iterator target_distribution =
      topic_distributions_.find(word);
  if (target_distribution == topic_distributions_.end()) {
    return zero_distribution_;
  }
  return target_distribution->second;
}

const TopicProbDistribution&
LDAAccumulativeModel::GetGlobalTopicDistribution() const {
  return global_distribution_;
}

void LDAAccumulativeModel::AppendAsString(std::ostream& out) const {
  for (map<string, TopicProbDistribution>::const_iterator iter =
           topic_distributions_.begin();
       iter != topic_distributions_.end();
       ++iter) {
    out << iter->first << "\t";
    for (int topic = 0; topic < num_topics(); ++topic) {
      out << (iter->second)[topic]
          << ((topic < num_topics() - 1) ? " " : "\n");
    }
  }
}

}  // namespace learning_lda
