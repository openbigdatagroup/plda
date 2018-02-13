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
#include <iostream>
#include <regex>
#include <pqxx/pqxx>
#include <cstdlib>


#include "common.h"
#include "document.h"
#include "model.h"
#include "accumulative_model.h"
#include "sampler.h"
#include "cmd_flags.h"

#define STATUS_LDA_ANALYSIS 5
#define STATUS_LDA_ANALYSIS 6
#define STATUS_COMPLETE  6

namespace learning_lda {

    using std::ifstream;
    using std::ofstream;
    using std::istringstream;
    using std::set;
    using std::map;
    using std::cout;
    using std::endl;
    using std::regex_match;
    using std::regex;
    using namespace pqxx;

    int utf8_strlen(const string& str)
    {
        int c,i,ix,q;
        for (q=0, i=0, ix=str.length(); i < ix; i++, q++)
        {
            c = (unsigned char) str[i];
            if      (c>=0   && c<=127) i+=0;
            else if ((c & 0xE0) == 0xC0) i+=1;
            else if ((c & 0xF0) == 0xE0) i+=2;
            else if ((c & 0xF8) == 0xF0) i+=3;
                //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
                //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
            else return 0;//invalid utf8
        }
        return q;
    }

    int LoadAndInitTrainingCorpus(int pk,
                                  int num_topics,
                                  LDACorpus* corpus,
                                  map<string, int>* word_index_map) {
        string const base_topic_req_sql = "SELECT target_url_is_included, target_url, min_word_length, get_english "
                "from topic_modeling_request where id = ?  and status = ?";
        string const base_req_lda_data_sql = "SELECT * from topic_modeling_lda_data where topic_request_id = ?";
        string const base_req_update_sql = "UPDATE topic_modeling_request set status = 6 where id = ?";
        string const base_page_topic_rel_sql = "SELECT pagedata_id from topic_modeling_url_page_data_topic_request "
                "where topicmodelingrequest_id = ?";
        string const base_page_sql = "SELECT word_dict from topic_modeling_url_page_data where id = ?";
        string sql;
        unsigned long pos;
        regex alphabet(".*[a-z].*");
        string const base_connection_str = "dbname = lm_backend user = lm_admin password = 1qazxsw2 hostaddr = ? "
                "port = 5432";
        string connection_str = base_connection_str;

        corpus->clear();
        word_index_map->clear();

        const char* django_setting = std::getenv("DJANGO_SETTINGS_MODULE");
        pos = connection_str.find('?');

        if (django_setting == NULL){
            connection_str.replace(pos, 1, "127.0.0.1");
        }
        else if (strcmp(django_setting, "lm_backend.settings_stage")){
            connection_str.replace(pos, 1, "192.168.7.50");
        }
        else if (strcmp(django_setting, "lm_backend.settings_prod")){
            connection_str.replace(pos, 1, "192.168.7.19");
        }
        else{
            connection_str.replace(pos, 1, "127.0.0.1");
        }

        connection C(connection_str);
        if (C.is_open()) {
            cout << "Opened database successfully: " << C.dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
            return 1;
        }
        /* Create a non-transactional object. */
        nontransaction N(C);

        /* Create SQL statement */
        sql = base_topic_req_sql;
        pos = sql.find('?');
        sql.replace(pos, 1, std::to_string(pk));
        pos = sql.find('?');
        sql.replace(pos, 1, std::to_string(STATUS_LDA_ANALYSIS));

        /* Execute SQL query */
        result requests( N.exec( sql ));

        /* List down all the records */
        bool request_exits = false;
        bool target_url_is_included;
        string target_url;
        int min_word_length;
        bool get_english;
        for (result::const_iterator c = requests.begin(); c != requests.end(); ++c) {
            request_exits = true;
            target_url_is_included = c[0].as<bool>();
            if(!c[1].is_null()){
                target_url = c[1].as<string>();
            }
            min_word_length = c[2].as<int>();
            get_english = c[3].as<bool>();
            break;
        }

        if (!request_exits){
            C.disconnect();
            cout << "uncompleted request " << pk << " does not exist" << endl;
            return 1;
        }

        sql = base_req_lda_data_sql;
        pos = sql.find('?');
        sql.replace(pos, 1, to_string(pk));
        result lda_data( N.exec( sql ));
        N.commit();

        /* Create a transactional object. */

        if (false){


            work W(C);

            /* if lda result exists, it means it is completed */
            for (result::const_iterator c = lda_data.begin(); c != lda_data.end(); ++c) {
                /* Create  SQL UPDATE statement */
                sql = base_req_update_sql;
                pos = sql.find('?');
                sql.replace(pos, 1, std::to_string(pk));
                /* Execute SQL query */
                W.exec( sql );
                W.commit();
                C.disconnect ();
                cout << "request " << pk << " is already completed" << endl;
                return 1;
            }

        }

        sql = base_page_topic_rel_sql;
        pos = sql.find('?');
        sql.replace(pos, 1, std::to_string(pk));
        nontransaction N2(C);
        result page_topic_rel( N2.exec( sql ));
        string word_dict;

        for (result::const_iterator c = page_topic_rel.begin(); c != page_topic_rel.end(); ++c) {
            int page_id = c[0].as<int>();
            sql = base_page_sql;
            pos = sql.find('?');
            sql.replace(pos, 1, std::to_string(page_id));
            result page_data(N2.exec(sql));
            for (result::const_iterator c2 = page_data.begin(); c2 != page_data.end(); ++c2) {
                string word_dict = c2[0].as<string>();
                istringstream ss(word_dict);
                string word;
                string count_str;
                int count;
                int document_total_count = 0;
                DocumentWordTopicsPB document;
                while (ss >> word >> count_str){
                    if (word.at(0) == '{'){
                        word = word.substr(2, word.size() - 4);
                    }
                    else{
                        word = word.substr(1, word.size() - 3);
                    }

                    if(utf8_strlen(word) < min_word_length){
                        continue;
                    }

                    if(!get_english && regex_match(word, alphabet)){
                        continue;
                    }

                    count_str.erase(count_str.size() - 1, 1);
                    count = atoi(count_str.c_str());
                    document_total_count += count;
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

                    if (document_total_count > 0){
                        document.add_wordtopics(word, word_index, topics);
                    }
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

    int pk = 118;
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp(argv[i], "--pk")) {
            std::istringstream(argv[i + 1]) >> pk;
            ++i;
        }
    }

    LDACmdLineFlags flags;
    flags.ParseCmdFlags(argc, argv);
    if (!flags.CheckTrainingValidity()) {
        return -1;
    }
    srand(time(NULL));
    LDACorpus corpus;
    map<string, int> word_index_map;
    CHECK_GT(LoadAndInitTrainingCorpus(pk,
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
