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

  ./lda           \
  --num_topics 2 \
  --alpha 0.1    \
  --beta 0.01                                           \
  --training_data_file ./testdata/test_data.txt \
  --model_file /tmp/lda_model.txt                       \
  --burn_in_iterations 100                              \
  --total_iterations 150
*/

#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <map>

#include "common.h"
#include "document.h"
#include "model.h"
#include "accumulative_model.h"
#include "sampler.h"
#include "cmd_flags.h"

namespace learning_lda {

using std::ifstream;
using std::ofstream;
using std::istringstream;
using std::set;
using std::map;

int LoadAndInitTrainingCorpus(const string& corpus_file,
                              int num_topics,
                              LDACorpus* corpus,
                              map<string, int>* word_index_map) {
  corpus->clear();
  word_index_map->clear();
  ifstream fin(corpus_file.c_str());
  string line;
  while (getline(fin, line)) {  // Each line is a training document.
    if (line.size() > 0 &&      // Skip empty lines.
        line[0] != '\r' &&      // Skip empty lines.
        line[0] != '\n' &&      // Skip empty lines.
        line[0] != '#') {       // Skip comment lines.
      istringstream ss(line);
      DocumentWordTopicsPB document;
      string word;
      int count;
      while (ss >> word >> count) {  // Load and init a document.
        vector<int32> topics;
        for (int i = 0; i < count; ++i) {
          topics.push_back(RandInt(num_topics));
        }
        int word_index;
        map<string, int>::const_iterator iter = word_index_map->find(word);
        if (iter == word_index_map->end()) {
          word_index = word_index_map->size();
          (*word_index_map)[word] = word_index;
        } else {
          word_index = iter->second;
        }
        document.add_wordtopics(word, word_index, topics);
      }
      corpus->push_back(new LDADocument(document, num_topics));
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

}  // namespace learning_lda

int main(int argc, char** argv) {
  using learning_lda::LDACorpus;
  using learning_lda::LDAModel;
  using learning_lda::LDAAccumulativeModel;
  using learning_lda::LDASampler;
  using learning_lda::LDADocument;
  using learning_lda::LoadAndInitTrainingCorpus;
  using learning_lda::LDACmdLineFlags;
  using std::list;

  LDACmdLineFlags flags;
  flags.ParseCmdFlags(argc, argv);
  if (!flags.CheckTrainingValidity()) {
    return -1;
  }
  srand(time(NULL));
  LDACorpus corpus;
  map<string, int> word_index_map;
  CHECK_GT(LoadAndInitTrainingCorpus(flags.training_data_file_,
                                     flags.num_topics_,
                                     &corpus, &word_index_map), 0);
  LDAModel model(flags.num_topics_, word_index_map);
  LDAAccumulativeModel accum_model(flags.num_topics_, word_index_map.size());
  LDASampler sampler(flags.alpha_, flags.beta_, &model, &accum_model);

  sampler.InitModelGivenTopics(corpus);

  for (int iter = 0; iter < flags.total_iterations_; ++iter) {
    std::cout << "Iteration " << iter << " ...\n";
    if (flags.compute_likelihood_ == "true") {
      double loglikelihood = 0;
      for (list<LDADocument*>::const_iterator iterator = corpus.begin();
           iterator != corpus.end();
           ++iterator) {
        loglikelihood += sampler.LogLikelihood(*iterator);
      }
      std::cout << "Loglikelihood: " << loglikelihood << std::endl;
    }
    sampler.DoIteration(&corpus, true, iter < flags.burn_in_iterations_);
  }
  accum_model.AverageModel(
      flags.total_iterations_ - flags.burn_in_iterations_);

  FreeCorpus(&corpus);

  std::ofstream fout(flags.model_file_.c_str());
  accum_model.AppendAsString(word_index_map, fout);

  return 0;
}
