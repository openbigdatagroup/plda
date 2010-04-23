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

LDAAccumulativeModel::LDAAccumulativeModel(int num_topics, int vocab_size) {
  CHECK_LT(1, num_topics);
  CHECK_LT(1, vocab_size);
  global_distribution_.resize(num_topics, 0);
  zero_distribution_.resize(num_topics, 0);
  topic_distributions_.resize(vocab_size);
  for (int i = 0; i < vocab_size; ++i) {
    topic_distributions_[i].resize(num_topics, 0);
  }
}

// Accumulate a model into accumulative_topic_distributions_ and
// accumulative_global_distributions_.
void LDAAccumulativeModel::AccumulateModel(const LDAModel& source_model) {
  CHECK_EQ(num_topics(), source_model.num_topics());
  for (LDAModel::Iterator iter(&source_model); !iter.Done(); iter.Next()) {
    const TopicCountDistribution& source_dist = iter.Distribution();
    TopicProbDistribution* dest_dist = &(topic_distributions_[iter.Word()]);
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
  for (vector<TopicProbDistribution>::iterator iter =
           topic_distributions_.begin();
       iter != topic_distributions_.end();
       ++iter) {
    TopicProbDistribution& dist = *iter;
    for (int k = 0; k < num_topics(); ++k) {
      dist[k] /= num_accumulations;
    }
  }
  for (int k = 0; k < num_topics(); ++k) {
    global_distribution_[k] /= num_accumulations;
  }
}

const TopicProbDistribution& LDAAccumulativeModel::GetWordTopicDistribution(
    int word) const {
  return topic_distributions_[word];
}

const TopicProbDistribution&
LDAAccumulativeModel::GetGlobalTopicDistribution() const {
  return global_distribution_;
}

void LDAAccumulativeModel::AppendAsString(const map<string, int>& word_index_map,
                                          std::ostream& out) const {
  vector<string> index_word_map(word_index_map.size());
  for (map<string, int>::const_iterator iter = word_index_map.begin();
       iter != word_index_map.end(); ++iter) {
    index_word_map[iter->second] = iter->first;
  }
  for (int i = 0; i < topic_distributions_.size(); ++i) {
    out << index_word_map[i] << "\t";
    for (int topic = 0; topic < num_topics(); ++topic) {
      out << topic_distributions_[i][topic]
          << ((topic < num_topics() - 1) ? " " : "\n");
    }
  }
}

}  // namespace learning_lda
