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
      iterator_(0) { }

LDAModel::Iterator::~Iterator() { }

void LDAModel::Iterator::Next() {
  CHECK(!Done());
  ++iterator_;
}

bool LDAModel::Iterator::Done() const {
  return iterator_ == parent_->topic_distributions_.size();
}

int LDAModel::Iterator::Word() const {
  CHECK(!Done());
  return iterator_;
}

// Returns the current word's distribution.
const TopicCountDistribution& LDAModel::Iterator::Distribution() const {
  CHECK(!Done());
  return parent_->GetWordTopicDistribution(iterator_);
}

LDAModel::LDAModel(
    int num_topics, const map<string, int>& word_index_map) {
  int vocab_size = word_index_map.size();
  memory_alloc_.resize(((int64)(num_topics)) * ((int64) vocab_size + 1), 0);
  // topic_distribution and global_distribution are just accessor pointers
  // and are not responsible for allocating/deleting memory.
  topic_distributions_.resize(vocab_size);
  global_distribution_.Reset(
      &memory_alloc_[0] + (int64)vocab_size * num_topics,
      num_topics);
  for (int i = 0; i < vocab_size; ++i) {
    topic_distributions_[i] =
        TopicCountDistribution(&memory_alloc_[0] + num_topics * i,
                               num_topics);
  }
  word_index_map_ = word_index_map;
}

const TopicCountDistribution& LDAModel::GetWordTopicDistribution(
    int word) const {
  return topic_distributions_[word];
}

const TopicCountDistribution&
LDAModel::GetGlobalTopicDistribution() const {
  return global_distribution_;
}

void LDAModel::IncrementTopic(int word,
                              int topic,
                              int64 count) {
  CHECK_GT(num_topics(), topic);
  CHECK_GT(num_words(), word);

  topic_distributions_[word][topic] += count;
  global_distribution_[topic] += count;
  CHECK_LE(0, topic_distributions_[word][topic]);
}

void LDAModel::ReassignTopic(int word,
                             int old_topic,
                             int new_topic,
                             int64 count) {
  IncrementTopic(word, old_topic, -count);
  IncrementTopic(word, new_topic, count);
}

void LDAModel::AppendAsString(std::ostream& out) const {
  vector<string> index_word_map(word_index_map_.size());
  for (map<string, int>::const_iterator iter = word_index_map_.begin();
       iter != word_index_map_.end(); ++iter) {
    index_word_map[iter->second] = iter->first;
  }
  for (LDAModel::Iterator iter(this); !iter.Done(); iter.Next()) {
    out << index_word_map[iter.Word()] << "\t";
    for (int topic = 0; topic < num_topics(); ++topic) {
      out << iter.Distribution()[topic]
          << ((topic < num_topics() - 1) ? " " : "\n");
    }
  }
}

LDAModel::LDAModel(std::istream& in, map<string, int>* word_index_map) {
  word_index_map_.clear();
  memory_alloc_.clear();
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
      while (ss >> count_float) {
        memory_alloc_.push_back((int64)count_float);
      }
      int size = word_index_map_.size();
      word_index_map_[word] = size;
    }
  }
  int vocab_size = word_index_map_.size();
  int num_topics = memory_alloc_.size() / vocab_size;
  memory_alloc_.resize(((int64)(num_topics)) * ((int64) vocab_size + 1), 0);
  // topic_distribution and global_distribution are just accessor pointers
  // and are not responsible for allocating/deleting memory.
  topic_distributions_.resize(vocab_size);
  global_distribution_.Reset(
      &memory_alloc_[0] + (int64)vocab_size * num_topics,
      num_topics);
  for (int i = 0; i < vocab_size; ++i) {
    topic_distributions_[i] =
        TopicCountDistribution(&memory_alloc_[0] + num_topics * i,
                               num_topics);
  }
  for (int i = 0; i < vocab_size; ++i) {
    for (int j = 0; j < num_topics; ++j) {
      global_distribution_[j] += topic_distributions_[i][j];
    }
  }
  *word_index_map = word_index_map_;
}
}  // namespace learning_lda
