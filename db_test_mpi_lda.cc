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

#include <pqxx/pqxx>
#include <cstdlib>
#include <cpp_redis/cpp_redis>
#include <chrono>
#include <thread>
#include <unistd.h>

#include <regex.h>

#include "common.h"
#include "document.h"
#include "model.h"
#include "accumulative_model.h"
#include "sampler.h"
#include "cmd_flags.h"

#define STATUS_LDA_ANALYSIS 5
#define STATUS_LDA_ANALYSIS 6
#define STATUS_COMPLETE  6

#define LOCK_NAME "lda_analysis"
#define LOCK_TIMEOUT 300
#define REDIS_LDA_HASH_NAME "lda_workers"

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


using namespace pqxx;
using namespace std::literals::chrono_literals;

namespace learning_lda {

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
            connection &C,
            int pk,
            bool target_url_is_included,
            string &target_url,
            int min_word_length,
            bool get_english,
            int num_topics,
            int myid, int pnum, LDACorpus* corpus, set<string>* words) {

        string const base_page_topic_rel_sql = "SELECT pagedata_id from topic_modeling_url_page_data_topic_request "
                "where topicmodelingrequest_id = ?";
        string const base_page_sql = "SELECT word_dict from topic_modeling_url_page_data where id = ?";
        string sql;
        unsigned long pos;
        regex_t reg;
        regcomp(&reg, ".*[a-z].*", REG_NOSUB | REG_EXTENDED);

        corpus->clear();
        int index = 0;

        sql = base_page_topic_rel_sql;
        pos = sql.find('?');
        sql.replace(pos, 1, to_string(pk));
        nontransaction N(C);
        result page_topic_rel( N.exec( sql ));
        string word_dict;

        for (result::const_iterator c = page_topic_rel.begin(); c != page_topic_rel.end(); ++c) {
            int page_id = c[0].as<int>();
            sql = base_page_sql;
            pos = sql.find('?');
            sql.replace(pos, 1, to_string(page_id));
            result page_data(N.exec(sql));
            for (result::const_iterator c2 = page_data.begin(); c2 != page_data.end(); ++c2) {
                string word_dict = c2[0].as<string>();
                istringstream ss(word_dict);
                if (index % pnum == myid) {
                    // This is a document that I need to store in local memory.
                    DocumentWordTopicsPB document;
                    string word;
                    string count_str;
                    int count;
                    set<string> words_in_document;
                    while (ss >> word >> count_str) {  // Load and init a document.
                        if (word.at(0) == '{'){
                            word = word.substr(2, word.size() - 4);
                        }
                        else{
                            word = word.substr(1, word.size() - 3);
                        }

                        if(utf8_strlen(word) < min_word_length){
                            continue;
                        }

                        if(!get_english && regexec(&reg, word.c_str(), 0, NULL, 0) != REG_NOMATCH){
                            continue;
                        }

                        count_str.erase(count_str.size() - 1, 1);
                        count = atoi(count_str.c_str());

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
                    string count_str;
                    while (ss >> word >> count_str) {  // Only fill words into word_set
                        if (word.at(0) == '{'){
                            word = word.substr(2, word.size() - 4);
                        }
                        else{
                            word = word.substr(1, word.size() - 3);
                        }

                        if(utf8_strlen(word) < min_word_length){
                            continue;
                        }

                        if(!get_english && regexec(&reg, word.c_str(), 0, NULL, 0) != REG_NOMATCH){
                            continue;
                        }

                        words->insert(word);
                    }
                }
                index++;
            }
        }
        N.commit();
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

std::string random_string( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
                "0123456789"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar );
    return str;
}

void lda_redis_lock(cpp_redis::client& client, const string& token){
    while(true){
        auto locking_ret = client.setnx(LOCK_NAME, token);
        client.commit();
        if(locking_ret.get()){
            client.pexpire(LOCK_NAME, LOCK_TIMEOUT * 1000);
            break;
        }
        else
            std::this_thread::sleep_for(5s);
    }
}

void lda_redis_unlock(cpp_redis::client& client, const string& token){
    vector<string> deleting_keys;
    deleting_keys.push_back(LOCK_NAME);

    auto lock_token_get = client.get(LOCK_NAME);
    client.commit();
    auto lock_token_get_reply = lock_token_get.get();
    if(!lock_token_get_reply.is_null()){
        string lock_token_str = lock_token_get_reply.as_string();
        std::cout<< "acutal token" << lock_token_str << std::endl;
        if (lock_token_str == token){
            client.del(deleting_keys);
            client.commit();
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

    //int pks[3] = {118, 258, 117};
    string const base_topic_req_sql = "SELECT target_url_is_included, target_url, min_word_length, get_english "
            "from topic_modeling_request where id = ?  and status = ?";
    string const base_req_lda_data_sql = "SELECT * from topic_modeling_lda_data where topic_request_id = ?";
    string const base_req_update_sql = "UPDATE topic_modeling_request set status = 6 where id = ?";
    string sql;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &pnum);

    cpp_redis::client client;

    char char_hostname[64];
    gethostname(char_hostname, 64);
    string hostname(char_hostname);

    vector<string> deleting_hash_keys;
    deleting_hash_keys.push_back(hostname);

    int pk;
    bool request_exits;
    bool target_url_is_included;
    string target_url;
    int min_word_length;
    bool get_english;

    int num_val_buffer[4];
    char url_buffer[2048];

    int pos;
    string connection_str = "dbname = lm_backend user = lm_admin password = 1qazxsw2 host = ? "
            "port = 5432";
    const char* django_setting = std::getenv("DJANGO_SETTINGS_MODULE");
    pos = connection_str.find('?');

    if (django_setting == NULL){
        connection_str.replace(pos, 1, "127.0.0.1");
        if(myid == 0){
            std::cout << "setting is localhost" << std::endl;
            client.connect("127.0.0.1", 6379);
            client.select(1);
            client.commit();
        }
    }
    else if (strcmp(django_setting, "lm_backend.settings_stage") == 0){
        connection_str.replace(pos, 1, "192.168.7.50");
        if(myid == 0){
            std::cout << "setting is stage" << std::endl;
            client.connect("192.168.6.201", 6379);
            client.select(1);
            client.commit();
        }
    }
    else if (strcmp(django_setting, "lm_backend.settings_prod") == 0){
        connection_str.replace(pos, 1, "192.168.7.19");
        if(myid == 0){
            std::cout << "setting is prod" << std::endl;
            client.connect("192.168.6.205", 6379);
            client.select(1);
            client.commit();
        }
    }
    else if (strcmp(django_setting, "docker") == 0){
        connection_str.replace(pos, 1, "docker.for.mac.host.internal");
        if(myid == 0){
            std::cout << "setting is docker" << std::endl;
            client.connect("docker.for.mac.host.internal", 6379);
            client.select(1);
            client.commit();
        }
    }
    else{
        connection_str.replace(pos, 1, "127.0.0.1");
        if(myid == 0){
            std::cout << "setting is null" << std::endl;
            client.connect("127.0.0.1", 6379);
            client.select(1);
            client.commit();
        }
    }
    connection C(connection_str);
    if (C.is_open()) {
        if(myid == 0){
            std::cout << "Opened database successfully: " << C.dbname() << std::endl;
        }
    } else {
        if(myid == 0){
            std::cout << "Can't open database" << std::endl;
        }
        return 1;
    }


    LDACmdLineFlags flags;
    flags.ParseCmdFlags(argc, argv);
    if (!flags.CheckParallelTrainingValidity()) {
        return -1;
    }
    for(int k=0; k< 6; k++){
        string token;
        if (myid == 0){
            token = random_string(16);
            lda_redis_lock(client, token);
            auto get_pk = client.hget(REDIS_LDA_HASH_NAME, hostname);
            client.commit();
            lda_redis_unlock(client, token);

            auto pk_get_reply = get_pk.get();

            if(pk_get_reply.is_null()){
                std::this_thread::sleep_for(30s);
                continue;
            }
            else{
                string pk_str = pk_get_reply.as_string();
                pk = atoi(pk_str.c_str());
            }

            std::cout<< "request received pk: " << pk << std::endl;

            /* Create a non-transactional object. */
            nontransaction N(C);

            /* Create SQL statement */
            sql = base_topic_req_sql;
            pos = sql.find('?');
            sql.replace(pos, 1, to_string(pk));
            pos = sql.find('?');
            sql.replace(pos, 1, to_string(STATUS_LDA_ANALYSIS));

            /* Execute SQL query */
            result requests( N.exec( sql ));
            request_exits = false;
            /* List down all the records */
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
                if (myid == 0){
                    std::cout << "uncompleted request " << pk << " does not exist" << std::endl;
                }
                continue;
            }

            sql = base_req_lda_data_sql;
            pos = sql.find('?');
            sql.replace(pos, 1, to_string(pk));
            result lda_data( N.exec( sql ));
            N.commit();

            /* Create a transactional object. */

            if (false){
                work W(C);

                bool is_already_done = false;

                /* if lda result exists, it means it is completed */
                for (result::const_iterator c = lda_data.begin(); c != lda_data.end(); ++c) {
                    /* Create  SQL UPDATE statement */
                    sql = base_req_update_sql;
                    pos = sql.find('?');
                    sql.replace(pos, 1, to_string(pk));
                    /* Execute SQL query */
                    W.exec( sql );
                    is_already_done = true;
                    std::cout << "request " << pk << " is already completed" << std::endl;
                    break;
                }
                W.commit();

                if (is_already_done)
                    lda_redis_lock(client, token);
                    client.hdel(REDIS_LDA_HASH_NAME, deleting_hash_keys);
                    client.commit();
                    lda_redis_unlock(client, token);
                    continue;
            }
            num_val_buffer[0] = pk;
            num_val_buffer[1] = min_word_length;
            if(target_url_is_included){
                num_val_buffer[2] = 1;
            }
            else{
                num_val_buffer[2] = 0;
            }

            if(get_english){
                num_val_buffer[3] = 1;
            }
            else{
                num_val_buffer[3] = 0;
            }

            for (int process_id = 1; process_id < pnum; ++process_id){
                MPI_Send(num_val_buffer, 4, MPI_INT, process_id, 0, MPI_COMM_WORLD);

                int copy_length = target_url.size();
                if (copy_length > 2048)
                    copy_length = 2048;
                strncpy(url_buffer, target_url.c_str(), copy_length);
                MPI_Send(target_url.c_str(), target_url.size(), MPI_CHAR, process_id, 1, MPI_COMM_WORLD);
            }

        }
        else{
            k = 0;
            MPI_Recv(num_val_buffer, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            pk = num_val_buffer[0];
            /* if pk is 0 that means terminate the process */
            if(pk == 0){
                break;
            }
            min_word_length = num_val_buffer[1];
            if(num_val_buffer[2] == 0){
                target_url_is_included = false;
            }
            else{
                target_url_is_included = true;
            }

            if(num_val_buffer[3] == 0){
                get_english = false;
            }
            else{
                get_english = true;
            }

            MPI_Recv(url_buffer, 2048, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            target_url = string(url_buffer);
        }

        srand(time(NULL));

        LDACorpus corpus;
        set<string> allwords;
        CHECK_GT(DistributelyLoadAndInitTrainingCorpus(C, pk, target_url_is_included, target_url, min_word_length,
                                                       get_english,
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

        if (myid == 0){
            lda_redis_lock(client, token);
            client.hdel(REDIS_LDA_HASH_NAME, deleting_hash_keys);
            client.commit();
            lda_redis_unlock(client, token);
            std::cout<< "request " << pk << " is finished" << std::endl;
            std::cout << "waiting for another request" << std::endl;
        }

    }
    C.disconnect();

    /* stop all other processes */
    if(myid == 0){
        client.disconnect();
        for (int process_id = 1; process_id < pnum; ++process_id){
            num_val_buffer[0] = 0;
            MPI_Send(num_val_buffer, 3, MPI_INT, process_id, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
