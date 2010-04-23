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

  ./infer \
  --alpha 0.1    \
  --beta 0.01                                           \
  --inference_data_file ./testdata/test_data.txt \
  --inference_result_file /tmp/inference_result.txt \
  --model_file /tmp/lda_model.txt                       \
  --burn_in_iterations 10                              \
  --total_iterations 15
*/
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include "common.h"
#include "document.h"
#include "model.h"
#include "sampler.h"
#include "cmd_flags.h"

int main(int argc, char** argv) {
  using learning_lda::LDACorpus;
  using learning_lda::LDAModel;
  using learning_lda::LDAAccumulativeModel;
  using learning_lda::LDASampler;
  using learning_lda::LDADocument;
  using learning_lda::LDACmdLineFlags;
  using learning_lda::DocumentWordTopicsPB;
  using learning_lda::RandInt;
  using std::ifstream;
  using std::ofstream;
  using std::istringstream;

  LDACmdLineFlags flags;
  flags.ParseCmdFlags(argc, argv);
  if (!flags.CheckInferringValidity()) {
    return -1;
  }
  srand(time(NULL));
  map<string, int> word_index_map;
  ifstream model_fin(flags.model_file_.c_str());
  LDAModel model(model_fin, &word_index_map);
  LDASampler sampler(flags.alpha_, flags.beta_, &model, NULL);
  ifstream fin(flags.inference_data_file_.c_str());
  ofstream out(flags.inference_result_file_.c_str());
  string line;
  while (getline(fin, line)) {  // Each line is a training document.
    if (line.size() > 0 &&      // Skip empty lines.
        line[0] != '\r' &&      // Skip empty lines.
        line[0] != '\n' &&      // Skip empty lines.
        line[0] != '#') {       // Skip comment lines.
      istringstream ss(line);
      DocumentWordTopicsPB document_topics;
      string word;
      int count;
      while (ss >> word >> count) {  // Load and init a document.
        vector<int32> topics;
        for (int i = 0; i < count; ++i) {
          topics.push_back(RandInt(model.num_topics()));
        }
        map<string, int>::const_iterator iter = word_index_map.find(word);
        if (iter != word_index_map.end()) {
          document_topics.add_wordtopics(word, iter->second, topics);
        }
      }
      LDADocument document(document_topics, model.num_topics());
      TopicProbDistribution prob_dist(model.num_topics(), 0);
      for (int iter = 0; iter < flags.total_iterations_; ++iter) {
        sampler.SampleNewTopicsForDocument(&document, false);
        if (iter >= flags.burn_in_iterations_) {
          const vector<int64>& document_distribution =
              document.topic_distribution();
          for (int i = 0; i < document_distribution.size(); ++i) {
            prob_dist[i] += document_distribution[i];
          }
        }
      }
      for (int topic = 0; topic < prob_dist.size(); ++topic) {
        out << prob_dist[topic] /
              (flags.total_iterations_ - flags.burn_in_iterations_)
            << ((topic < prob_dist.size() - 1) ? " " : "\n");
      }
    }
  }
}
