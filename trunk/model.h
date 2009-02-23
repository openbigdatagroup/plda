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

#ifndef _OPENSOURCE_GLDA_MODEL_H__
#define _OPENSOURCE_GLDA_MODEL_H__

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "common.h"

namespace learning_lda {

// The LDAModel class stores topic-word co-occurrence count vectors as
// well as a vector of global topic occurrence counts.  The global vector is
// the sum of the other vectors.  These vectors are precisely the components of
// an LDA model (as document-topic associations are not true model parameters).
//
// This class supports common operations on this sort of model, primarily in
// the form of assigning new topic occurrences to words, and in reassigning
// word occurrences from one topic to another.
//
// This class is not thread-safe.  Do not share an object of this
// class by multiple threads.
class LDAModel {
 public:
  // An iterator over a LDAModel.  Returns distributions in an arbitrary
  // order.  Altering the parent LDAModel in any way invalidates the
  // iterator, although this is not currently enforced.
  class Iterator {
   public:
    // Initializes the iterator for the model specified by parent.  parent must
    // exist and must not be modified for the lifetime of the iterator.
    explicit Iterator(const LDAModel* parent);

    ~Iterator();

    // Advances to the next word.
    void Next();

    // Returns true if we have finished iterating over the model.
    bool Done() const;

    // Returns the current word.
    const string& Word() const;

    // Returns the current word's distribution.
    const TopicCountDistribution& Distribution() const;

   private:
    const LDAModel* parent_;
    map<string, TopicCountDistribution>::const_iterator iterator_;
  };
  friend class Iterator;


  explicit LDAModel(int num_topics);
  ~LDAModel() {}

  // Returns the topic distribution for word.
  const TopicCountDistribution& GetWordTopicDistribution(
      const string& word) const;

  // Returns the global topic distribution.
  const TopicCountDistribution& GetGlobalTopicDistribution() const;

  // Increments the topic count for a particular word (or decrements, for
  // negative values of count).  Creates the word distribution if it doesn't
  // exist, even if the count is 0.
  void IncrementTopic(const string& word,
                      int topic,
                      int64 count);

  // Reassigns count occurrences of a word from old_topic to new_topic.
  void ReassignTopic(const string& word,
                     int old_topic,
                     int new_topic,
                     int64 count);

  // Returns the number of topics in the model.
  int num_topics() const { return global_distribution_.size(); }

  // Returns the number of words in the model (not including the global word).
  int num_words() const { return topic_distributions_.size(); }

  // Returns true if the given word is in the vocabulary of the model.
  bool IsWordInVocabulary(const string word) const {
    return topic_distributions_.find(word) != topic_distributions_.end();
  }

  // Checks that the model is sane.
  bool IsValid() const;

  // Output topic_distributions_ into human readable format.
  void AppendAsString(std::ostream& out) const;

  // Read word topic distribution and global distribution from iframe.
  void Load(std::istream& in);
 private:
  // If users query a word for its topic distribution via
  // GetWordTopicDistribution, but this word does not appear in the
  // training corpus, GetWordTopicDistribution returns
  // zero_distribution_.
  TopicCountDistribution zero_distribution_;

  // topic_distributions_["word"][k] counts the number of times that
  // word "word" and assigned topic k by a Gibbs sampling iteration.
  map<string, TopicCountDistribution> topic_distributions_;

  // global_distribution_[k] is the number of words in the training
  // corpus that are assigned by topic k.
  TopicCountDistribution global_distribution_;
};

}  // namespace learning_lda

#endif  // _OPENSOURCE_GLDA_MODEL_H__
