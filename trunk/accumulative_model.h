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

#ifndef _OPENSOURCE_GLDA_ACCUMULATIVE_MODEL_H__
#define _OPENSOURCE_GLDA_ACCUMULATIVE_MODEL_H__

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "common.h"
#include "model.h"

namespace learning_lda {

// LDAAccumulativeModel is used by LDASampler together with LDAModel.
// Each Gibbs sampling iteration after the burn-in period should (1)
// update the LDAModel object model_, and (3) accumulate model_ into
// the LDAAccumulativeModel object accumulated_model_.  After the last
// iteration of Gibbs sampling, we should average accumulative_model_
// by the number of iterations after the burn-in period.
class LDAAccumulativeModel {
 public:
  LDAAccumulativeModel(int num_topics, int vocab_size);
  ~LDAAccumulativeModel() {}

  // Accumulate a model into accumulative_topic_distributions_ and
  // accumulative_global_distributions_.
  void AccumulateModel(const LDAModel& model);

  // Divide accumulative_topic_distributions_ and
  // accumulative_global_distributions_ by num_estiamte_iterations.
  void AverageModel(int num_estiamte_iterations);

  // Returns the topic distribution for word.
  const TopicProbDistribution& GetWordTopicDistribution(
      int word) const;

  // Returns the global topic distribution.
  const TopicProbDistribution& GetGlobalTopicDistribution() const;

  // Returns the number of topics in the model.
  int num_topics() const { return global_distribution_.size(); }

  // Returns the number of words in the model (not including the global word).
  int num_words() const { return topic_distributions_.size(); }

  // Output accumulative_topic_distributions_ in human-readable
  // format.
  void AppendAsString(const map<string, int>& word_index_map, std::ostream& out) const;

 private:
  // Increments the topic count for a particular word (or decrements, for
  // negative values of count).  Creates the word distribution if it doesn't
  // exist, even if the count is 0.
  void IncrementTopic(int word,
                      int topic,
                      int64 count);

  // If users query a word for its topic distribution via
  // GetWordTopicDistribution, but this word does not appear in the
  // training corpus, GetWordTopicDistribution returns
  // zero_distribution_.
  TopicProbDistribution zero_distribution_;

  // The summation of P(word|topic) matrices and P(topic) vectors
  // estimated by Gibbs sampling iterations after the burn-in period.
  vector<TopicProbDistribution> topic_distributions_;
  TopicProbDistribution global_distribution_;
};

}  // namespace learning_lda

#endif  // _OPENSOURCE_GLDA_ACCUMULATIVE_MODEL_H__
