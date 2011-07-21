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
/*
  An example running of this program:

  mpiexec -n 2 ./mpi_lda           \
  --num_topics 2 \
  --alpha 0.1    \
  --beta 0.01                                           \
  --training_data_file ./testdata/test_data.txt \
  --model_file /tmp/lda_model.txt                       \
  --burn_in_iterations 100                              \
  --total_iterations 150
*/

#include "mpi.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <vector>
#include <sstream>
#include <string>

#include "common.h"
#include "document.h"
#include "model.h"
#include "accumulative_model.h"
#include "sampler.h"
#include "cmd_flags.h"

using std::ifstream;
using std::ofstream;
using std::istringstream;
using std::set;
using std::vector;
using std::list;
using std::map;
using std::sort;
using std::string;
using learning_lda::LDADocument;

namespace learning_lda {

// A wrapper of MPI_Allreduce. If the vector is over 32M, we allreduce part
// after part. This will save temporary memory needed.
void AllReduceTopicDistribution(int64* buf, int count) {
  static int kMaxDataCount = 1 << 22;
  static int datatype_size = sizeof(*buf);
  if (count > kMaxDataCount) {
    char* tmp_buf = new char[datatype_size * kMaxDataCount];
    for (int i = 0; i < count / kMaxDataCount; ++i) {
      MPI_Allreduce(reinterpret_cast<char*>(buf) +
             datatype_size * kMaxDataCount * i,
             tmp_buf,
             kMaxDataCount, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
      memcpy(reinterpret_cast<char*>(buf) +
             datatype_size * kMaxDataCount * i, tmp_buf,
             kMaxDataCount * datatype_size);
    }
    // If count is not divisible by kMaxDataCount, there are some elements left
    // to be reduced.
    if (count % kMaxDataCount > 0) {
      MPI_Allreduce(reinterpret_cast<char*>(buf)
               + datatype_size * kMaxDataCount * (count / kMaxDataCount),
               tmp_buf,
               count - kMaxDataCount * (count / kMaxDataCount), MPI_LONG_LONG, MPI_SUM,
               MPI_COMM_WORLD);
      memcpy(reinterpret_cast<char*>(buf)
               + datatype_size * kMaxDataCount * (count / kMaxDataCount),
               tmp_buf,
               (count - kMaxDataCount * (count / kMaxDataCount)) * datatype_size);
    }
    delete[] tmp_buf;
  } else {
    char* tmp_buf = new char[datatype_size * count];
    MPI_Allreduce(buf, tmp_buf, count, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
    memcpy(buf, tmp_buf, datatype_size * count);
    delete[] tmp_buf;
  }
}

class ParallelLDAModel : public LDAModel {
 public:
  ParallelLDAModel(int num_topic, const map<string, int>& word_index_map)
      : LDAModel(num_topic, word_index_map) {
  }
  void ComputeAndAllReduce(const LDACorpus& corpus) {
    for (list<LDADocument*>::const_iterator iter = corpus.begin();
         iter != corpus.end();
         ++iter) {
      LDADocument* document = *iter;
      for (LDADocument::WordOccurrenceIterator iter2(document);
           !iter2.Done(); iter2.Next()) {
        IncrementTopic(iter2.Word(), iter2.Topic(), 1);
      }
    }
    AllReduceTopicDistribution(&memory_alloc_[0], memory_alloc_.size());
  }
};

int DistributelyLoadAndInitTrainingCorpus(
    const string& corpus_file,
    int num_topics,
    int myid, int pnum, LDACorpus* corpus, set<string>* words) {
  corpus->clear();
  ifstream fin(corpus_file.c_str());
  string line;
  int index = 0;
  while (getline(fin, line)) {  // Each line is a training document.
    if (line.size() > 0 &&      // Skip empty lines.
        line[0] != '\r' &&      // Skip empty lines.
        line[0] != '\n' &&      // Skip empty lines.
        line[0] != '#') {       // Skip comment lines.
      istringstream ss(line);
      if (index % pnum == myid) {
        // This is a document that I need to store in local memory.
        DocumentWordTopicsPB document;
        string word;
        int count;
        set<string> words_in_document;
        while (ss >> word >> count) {  // Load and init a document.
          vector<int32> topics;
          for (int i = 0; i < count; ++i) {
            topics.push_back(RandInt(num_topics));
          }
          document.add_wordtopics(word, -1, topics);
          words_in_document.insert(word);
          words->insert(word);
        }
        if (words_in_document.size() > 0) {
          corpus->push_back(new LDADocument(document, num_topics));
        }
      } else {
        // This is a document that should be stored by other processors. I just
        // need to read the words and build the word set.
        string word;
        int count;
        while (ss >> word >> count) {  // Only fill words into word_set
          words->insert(word);
        }
      }
      index++;
    }
  }
  return corpus->size();
}

void FreeCorpus(LDACorpus* corpus) {
  for (list<LDADocument*>::iterator iter = corpus->begin();
       iter != corpus->end();
       ++iter) {
    if (*iter != NULL) {
      delete *iter;
      *iter = NULL;
    }
  }
}
}
int main(int argc, char** argv) {
  using learning_lda::LDACorpus;
  using learning_lda::LDAModel;
  using learning_lda::ParallelLDAModel;
  using learning_lda::LDASampler;
  using learning_lda::DistributelyLoadAndInitTrainingCorpus;
  using learning_lda::LDACmdLineFlags;
  int myid, pnum;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  MPI_Comm_size(MPI_COMM_WORLD, &pnum);

  LDACmdLineFlags flags;
  flags.ParseCmdFlags(argc, argv);
  if (!flags.CheckParallelTrainingValidity()) {
    return -1;
  }

  srand(time(NULL));

  LDACorpus corpus;
  set<string> allwords;
  CHECK_GT(DistributelyLoadAndInitTrainingCorpus(flags.training_data_file_,
                                     flags.num_topics_,
                                     myid, pnum, &corpus, &allwords), 0);
  std::cout << "Training data loaded" << std::endl;
  // Make vocabulary words sorted and give each word an int index.
  vector<string> sorted_words;
  map<string, int> word_index_map;
  for (set<string>::const_iterator iter = allwords.begin();
       iter != allwords.end(); ++iter) {
    sorted_words.push_back(*iter);
  }
  sort(sorted_words.begin(), sorted_words.end());
  for (int i = 0; i < sorted_words.size(); ++i) {
    word_index_map[sorted_words[i]] = i;
  }
  for (LDACorpus::iterator iter = corpus.begin(); iter != corpus.end();
       ++iter) {
    (*iter)->ResetWordIndex(word_index_map);
  }

  for (int iter = 0; iter < flags.total_iterations_; ++iter) {
    if (myid == 0) {
      std::cout << "Iteration " << iter << " ...\n";
    }
    ParallelLDAModel model(flags.num_topics_, word_index_map);
    model.ComputeAndAllReduce(corpus);
    LDASampler sampler(flags.alpha_, flags.beta_, &model, NULL);
    if (flags.compute_likelihood_ == "true") {
      double loglikelihood_local = 0;
      double loglikelihood_global = 0;
      for (list<LDADocument*>::const_iterator iter = corpus.begin();
           iter != corpus.end();
           ++iter) {
        loglikelihood_local += sampler.LogLikelihood(*iter);
      }
      MPI_Allreduce(&loglikelihood_local, &loglikelihood_global, 1, MPI_DOUBLE,
                    MPI_SUM, MPI_COMM_WORLD);
      if (myid == 0) {
        std::cout << "Loglikelihood: " << loglikelihood_global << std::endl;
      }
    }
    sampler.DoIteration(&corpus, true, false);
  }
  ParallelLDAModel model(flags.num_topics_, word_index_map);
  model.ComputeAndAllReduce(corpus);
  if (myid == 0) {
    std::ofstream fout(flags.model_file_.c_str());
    model.AppendAsString(fout);
  }
  FreeCorpus(&corpus);
  MPI_Finalize();
  return 0;
}
