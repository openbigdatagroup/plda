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

#ifndef _OPENSOURCE_GLDA_COMMON_H__
#define _OPENSOURCE_GLDA_COMMON_H__

#include <stdlib.h>

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>



// The CHECK_xxxx facilities, which generates a segmentation fault
// when a check is failed.  If the program is run within a debugger,
// the segmentation fault makes the debugger keeps track of the stack,
// which provides the context of the fail.
//
extern char kSegmentFaultCauser[];

#define CHECK(a) if (!(a)) {                                            \
    std::cerr << "CHECK failed "                                        \
              << __FILE__ << ":" << __LINE__ << "\n"                    \
              << #a << " = " << (a) << "\n";                            \
    *kSegmentFaultCauser = '\0';                                        \
  }                                                                     \

#define CHECK_EQ(a, b) if (!((a) == (b))) {                             \
    std::cerr << "CHECK_EQ failed "                                     \
              << __FILE__ << ":" << __LINE__ << "\n"                    \
              << #a << " = " << (a) << "\n"                             \
              << #b << " = " << (b) << "\n";                            \
    *kSegmentFaultCauser = '\0';                                        \
  }                                                                     \

#define CHECK_GT(a, b) if (!((a) > (b))) {                              \
    std::cerr << "CHECK_GT failed "                                     \
              << __FILE__ << ":" << __LINE__ << "\n"                    \
              << #a << " = " << (a) << "\n"                             \
              << #b << " = " << (b) << "\n";                            \
    *kSegmentFaultCauser = '\0';                                        \
  }                                                                     \

#define CHECK_LT(a, b) if (!((a) < (b))) {                              \
    std::cerr << "CHECK_LT failed "                                     \
              << __FILE__ << ":" << __LINE__ << "\n"                    \
              << #a << " = " << (a) << "\n"                             \
              << #b << " = " << (b) << "\n";                            \
    *kSegmentFaultCauser = '\0';                                        \
  }                                                                     \

#define CHECK_GE(a, b) if (!((a) >= (b))) {                             \
    std::cerr << "CHECK_GE failed "                                     \
              << __FILE__ << ":" << __LINE__ << "\n"                    \
              << #a << " = " << (a) << "\n"                             \
              << #b << " = " << (b) << "\n";                            \
    *kSegmentFaultCauser = '\0';                                        \
  }                                                                     \

#define CHECK_LE(a, b) if (!((a) <= (b))) {             \
    std::cerr << "CHECK_LE failed "                     \
              << __FILE__ << ":" << __LINE__ << "\n"    \
              << #a << " = " << (a) << "\n"             \
              << #b << " = " << (b) << "\n";            \
    *kSegmentFaultCauser = '\0';                        \
  }                                                     \
                                                        \


// The log facility, which makes it easy to leave of trace of your
// program.  The logs are classified according to their severity
// levels.  Logs of the level FATAL will cause a segmentation fault,
// which makes the debugger to keep track of the stack.
//
// Examples:
//   LOG(INFO) << iteration << "-th iteration ...";
//   LOG(FATAL) << "Probability value < 0 " << prob_value;
//
enum LogSeverity { INFO, WARNING, ERROR, FATAL };

class Logger {
 public:
  Logger(LogSeverity ls, const std::string& file, int line)
      : ls_(ls), file_(file), line_(line)
  {}
  std::ostream& stream() const {
    return std::cerr << file_ << " (" << line_ << ") : ";
  }
  ~Logger() {
    if (ls_ == FATAL) {
      *::kSegmentFaultCauser = '\0';
    }
  }
 private:
  LogSeverity ls_;
  std::string file_;
  int line_;
};

#define LOG(ls) Logger(ls, __FILE__, __LINE__).stream()

// Basis POD types.
typedef int                 int32;
#ifdef COMPILER_MSVC
typedef __int64             int64;
#else
typedef long long           int64;
#endif

// Frequently-used STL containers.
using std::list;
using std::map;
using std::string;
using std::vector;
using std::string;

// A dense vector of counts used for storing topic counts.
// No memory allocation here, just keep pointers.
class TopicCountDistribution {
 public:
  TopicCountDistribution()
      : distribution_(NULL), size_(0) {
  }
  TopicCountDistribution(int64* distribution, int size)
      : distribution_(distribution), size_(size) {
  }
  void Reset(int64* distribution, int size) {
    distribution_ = distribution;
    size_ = size;
  }
  int size() const { return size_; }
  inline int64& operator[](int index) const { return distribution_[index]; }
  void clear() { memset(distribution_, 0, sizeof(*distribution_) * size_); }
 private:
  int64* distribution_;
  int size_;
};

// A dense vector of probability values representing a discrete
// probability distribution, e.g., the topic distribution of a word.
typedef vector<double> TopicProbDistribution;

namespace learning_lda {

bool IsValidProbDistribution(const TopicProbDistribution& dist);

// The structure representing a document as a bag of words and the
// topic assigned to each occurrence of a word.  In term of Bayesian
// learning and LDA, the bag of words are ``observable'' data; the
// topic assignments are ``hidden'' data.
struct DocumentWordTopicsPB {
  // The document unique words list.
  vector<string> words_s_;
  vector<int> words_;
  // Each word occurrance's topic.
  //  wordtopics_.size() = num_words_in_document.
  //  words_.size() = num_unique_words_in_document.
  vector<int32> wordtopics_;
  // A map from words_ to wordtopics_.
  // For example:
  // The document: WORDS1:4  WORD2:2 WORD3:1
  // words_:                     WORD1 WORD2  WORD3
  // wordtopics_start_index_:     |       \      |
  // wordtopics_:                 0 3 4 0  0 3   1
  vector<int> wordtopics_start_index_;
  DocumentWordTopicsPB() { wordtopics_start_index_.push_back(0); }
  int words_size() const { return words_.size(); }
  int wordtopics_count(int word_index) const {
    return wordtopics_start_index_[word_index + 1] - wordtopics_start_index_[word_index];
  }
  int word_last_topic_index(int word_index) const {
    return wordtopics_start_index_[word_index + 1] - 1;
  }
  int word(int word_index) const { return words_[word_index]; }
  int32 wordtopics(int index) const { return wordtopics_[index]; }
  int32* mutable_wordtopics(int index) { return &wordtopics_[index]; }

  void add_wordtopics(const string& word_s,
                      int word, const vector<int32>& topics) {
    words_s_.push_back(word_s);
    words_.push_back(word);
    wordtopics_start_index_.pop_back();
    wordtopics_start_index_.push_back(wordtopics_.size());
    for (size_t i = 0; i < topics.size(); ++i) {
      wordtopics_.push_back(topics[i]);
    }
    wordtopics_start_index_.push_back(wordtopics_.size());
  }
  void CopyFrom(const DocumentWordTopicsPB& instance) { *this = instance; }
};

// Generate a random float value in the range of [0,1) from the
// uniform distribution.
inline double RandDouble() {
  return rand() / static_cast<double>(RAND_MAX);
}

// Generate a random integer value in the range of [0,bound) from the
// uniform distribution.
inline int RandInt(int bound) {
  // NOTE: Do NOT use rand() % bound, which does not approximate a
  // discrete uniform distribution will.
  return static_cast<int>(RandDouble() * bound);
}

// Returns a sample selected from a non-normalized probability distribution.
int GetAccumulativeSample(const vector<double>& distribution);


// Steaming output facilities.
std::ostream& operator << (std::ostream& out, vector<double>& v);

}  // namespace learning_lda

#endif  // _OPENSOURCE_GLDA_COMMON_H__
