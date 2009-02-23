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

#include "model.h"

#include <map>
#include <sstream>
#include <string>

namespace learning_lda {

// Start by pointing to the beginning of the parent model's topic distribution
// map.
LDAModel::Iterator::Iterator(const LDAModel* parent)
    : parent_(parent),
      iterator_(parent->topic_distributions_.begin()) { }

LDAModel::Iterator::~Iterator() { }

void LDAModel::Iterator::Next() {
  CHECK(!Done());
  ++iterator_;
}

bool LDAModel::Iterator::Done() const {
  return iterator_ == parent_->topic_distributions_.end();
}

const string& LDAModel::Iterator::Word() const {
  CHECK(!Done());
  return iterator_->first;
}

// Returns the current word's distribution.
const TopicCountDistribution& LDAModel::Iterator::Distribution() const {
  CHECK(!Done());
  return parent_->GetWordTopicDistribution(Word());
}

LDAModel::LDAModel(int num_topics) {
  global_distribution_.resize(num_topics, 0);
  zero_distribution_.resize(num_topics, 0);
  CHECK(IsValid());
}

const TopicCountDistribution& LDAModel::GetWordTopicDistribution(
    const string& word) const {
  map<string, TopicCountDistribution>::const_iterator target_distribution =
      topic_distributions_.find(word);
  if (target_distribution == topic_distributions_.end()) {
    return zero_distribution_;
  }
  return target_distribution->second;
}

const TopicCountDistribution&
LDAModel::GetGlobalTopicDistribution() const {
  return global_distribution_;
}

void LDAModel::IncrementTopic(const string& word,
                                   int topic,
                                   int64 count) {
  CHECK_GT(num_topics(), topic);

  // Search for the target distribution and create it if it doesn't exist.
  map<string, TopicCountDistribution>::iterator target_distribution =
      topic_distributions_.find(word);
  if (target_distribution == topic_distributions_.end()) {
    topic_distributions_[word].resize(num_topics());
  }

  topic_distributions_[word][topic] += count;
  global_distribution_[topic] += count;
  CHECK_LE(0, topic_distributions_[word][topic]);
}

void LDAModel::ReassignTopic(const string& word,
                                  int old_topic,
                                  int new_topic,
                                  int64 count) {
  IncrementTopic(word, old_topic, -count);
  IncrementTopic(word, new_topic, count);
}

bool LDAModel::IsValid() const {
  // We LOG(ERROR) instead of CHECK here, where possible, because we want to
  // make sure that we catch every error we can.
  bool success = true;

  // We're going to accumulate all the non-global distributions into this one,
  // and at the end check to make sure that it's the same as the global
  // distribution.
  TopicCountDistribution check_distribution(num_topics());

  for (Iterator iterator(this); !iterator.Done(); iterator.Next()) {
    CHECK_EQ(num_topics(), iterator.Distribution().size());
    for (int i = 0; i < num_topics(); ++i) {
      check_distribution[i] += iterator.Distribution()[i];

      if (iterator.Distribution()[i] < 0) {
        success = false;
        LOG(ERROR) << "Word " << iterator.Word() << " in topic " << i
                   << " has negative count " << iterator.Distribution()[i]
                   << " in a non-diff model.";
      }
    }
  }
  // Make sure that the global distribution and the newly-calculated sum are
  // equal.
  for (int i = 0; i < num_topics(); ++i) {
    if (check_distribution[i] != GetGlobalTopicDistribution()[i]) {
      success = false;
      LOG(ERROR) << "For topic " << i << ":  sum of word counts is "
                 << check_distribution[i] << ", but global count is "
                 << GetGlobalTopicDistribution()[i] << ".";
    }
  }

  return success;
}

void LDAModel::AppendAsString(std::ostream& out) const {
  for (LDAModel::Iterator iter(this); !iter.Done(); iter.Next()) {
    out << iter.Word() << "\t";
    for (int topic = 0; topic < num_topics(); ++topic) {
      out << iter.Distribution()[topic]
          << ((topic < num_topics() - 1) ? " " : "\n");
    }
  }
}

void LDAModel::Load(std::istream& in) {
  string line;
  while (getline(in, line)) {  // Each line is a training document.
    if (line.size() > 0 &&      // Skip empty lines.
        line[0] != '\r' &&      // Skip empty lines.
        line[0] != '\n' &&      // Skip empty lines.
        line[0] != '#') {       // Skip comment lines.
      std::istringstream ss(line);
      string word;
      double count_float;
      CHECK(ss >> word);
      TopicCountDistribution word_distribution;
      while (ss >> count_float) {
        word_distribution.push_back((int64)count_float);
      }
      int distribution_size = word_distribution.size();
      if (num_topics() == 0) {
        global_distribution_.resize(distribution_size, 0);
        zero_distribution_.resize(distribution_size, 0);
      } else {
        CHECK_EQ(num_topics(), distribution_size);
      }
      topic_distributions_[word] = word_distribution;
      for (int i = 0; i < distribution_size; ++i) {
        global_distribution_[i] += word_distribution[i];
      }
    }
  }
}
}  // namespace learning_lda
