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

#ifndef _OPENSOURCE_GLDA_SAMPLER_H__
#define _OPENSOURCE_GLDA_SAMPLER_H__

#include "common.h"
#include "document.h"
#include "model.h"
#include "accumulative_model.h"

namespace learning_lda {

// LDASampler trains LDA models and computes statistics about documents in
// LDA models.
class LDASampler {
 public:
  // alpha and beta are the Gibbs sampling symmetric hyperparameters.
  // model is the model to use.
  LDASampler(double alpha, double beta,
             LDAModel* model,
             LDAAccumulativeModel* accum_model);

  ~LDASampler() {}

  // Given a corpus, whose every document have been initialized (i.e.,
  // every word occurrences has a (randomly) assigned topic,
  // initialize model_ to count the word-topic co-occurrences.
  void InitModelGivenTopics(const LDACorpus& corpus);

  // Performs one round of Gibbs sampling on documents in the corpus
  // by invoking SampleNewTopicsForDocument(...).  If we are to train
  // a model given training data, we should set train_model to true,
  // and the algorithm updates model_ during Gibbs sampling.
  // Otherwise, if we are to sample the latent topics of a query
  // document, we should set train_model to false.  If train_model is
  // true, burn_in indicates should we accumulate the current estimate
  // to accum_model_.  For the first certain number of iterations,
  // where the algorithm has not converged yet, you should set burn_in
  // to false.  After that, we should set burn_in to true.
  void DoIteration(LDACorpus* corpus, bool train_model, bool burn_in);

  // Performs one round of Gibbs sampling on a document.  Updates
  // document's topic assignments.  For learning, update_model_=true,
  // for sampling topics of a query, update_model_==false.
  void SampleNewTopicsForDocument(LDADocument* document,
                                  bool update_model);

  // The core of the Gibbs sampling process.  Compute the full conditional
  // posterior distribution of topic assignments to the indicated word.
  //
  // That is, holding all word-topic assignments constant, except for the
  // indicated one, compute a non-normalized probability distribution over
  // topics for the indicated word occurrence.
  void GenerateTopicDistributionForWord(const LDADocument& document,
      int word, int current_word_topic, bool train_model,
      vector<double>* distribution) const;

  // Computes the log likelihood of a document.
  double LogLikelihood(LDADocument* document) const;

 private:
  const double alpha_;
  const double beta_;
  LDAModel* model_;
  LDAAccumulativeModel* accum_model_;
};


}  // namespace learning_lda

#endif  // _OPENSOURCE_GLDA_SAMPLER_H__
