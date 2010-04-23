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

#ifndef _OPENSOURCE_GLDA_DOCUMENT_H__
#define _OPENSOURCE_GLDA_DOCUMENT_H__

#include <string>
#include <utility>
#include <vector>

#include "common.h"

namespace learning_lda {

class DocumentWordTopicsPB;

// Stores a document as a bag of words and provides methods for interacting
// with Gibbs LDA models.
class LDADocument {
 public:
  // An iterator over all of the word occurrences in a document.
  class WordOccurrenceIterator {
   public:
    // Intialize the WordOccurrenceIterator for a document.
    explicit WordOccurrenceIterator(LDADocument* parent);
    ~WordOccurrenceIterator();

    // Returns true if we are done iterating.
    bool Done();

    // Advances to the next word occurrence.
    void Next();

    // Returns the topic of the current occurrence.
    int Topic();

    // Changes the topic of the current occurrence.
    void SetTopic(int new_topic);

    // Returns the word of the current occurrence.
    int Word();

   private:
    // If the current word has no occurrences, advance until reaching a word
    // that does have occurrences or the end of the document.
    void SkipWordsWithoutOccurrences();

    LDADocument* parent_;
    int word_index_;
    int word_topic_index_;
  };
  friend class WordOccurrenceIterator;

  // Initializes a document from a DocumentWordTopicsPB. Usually, this
  // constructor is used in training an LDA model, because the
  // initialization phase creates the model whose vocabulary covers
  // all words appear in the training data.
  LDADocument(const DocumentWordTopicsPB& topics, int num_topics);

  virtual ~LDADocument();

  // Returns the document's topic associations.
  const DocumentWordTopicsPB& topics() const {
    return *topic_assignments_;
  }

  // Returns the document's topic occurrence counts.
  const vector<int64>& topic_distribution() const {
    return topic_distribution_;
  }

  void ResetWordIndex(const map<string, int>& word_index_map);

  string DebugString();
 protected:
  DocumentWordTopicsPB*  topic_assignments_;
  vector<int64> topic_distribution_;

  // Count topic occurrences in topic_assignments_ and stores the
  // result in topic_distribution_.
  void CountTopicDistribution();
};

typedef list<LDADocument*> LDACorpus;

}  // namespace learning_lda

#endif  // _OPENSOURCE_GLDA_DOCUMENT_H__
