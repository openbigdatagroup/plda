#include <iostream>
#include <string>
#include <pqxx/pqxx>

#define STATUS_LDA_ANALYSIS 5
#define STATUS_LDA_ANALYSIS 6
#define STATUS_COMPLETE  6

using namespace std;
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

int main(int argc, char* argv[]) {
    string const base_topic_req_sql = "SELECT target_url_is_included, target_url from topic_modeling_request "
            "where id = ?  and status = ?";
    string const base_req_lda_data_sql = "SELECT * from topic_modeling_lda_data where topic_request_id = ?";
    string const base_req_update_sql = "UPDATE topic_modeling_request set status = 6 where id = ?";
    string const base_page_topic_rel_sql = "SELECT pagedata_id from topic_modeling_url_page_data_topic_request "
            "where topicmodelingrequest_id = ?";
    string const base_page_sql = "SELECT word_dict from topic_modeling_url_page_data where id = ?";
    string sql;
    int pk = 118;
    unsigned long pos;
    try {
        connection C("dbname = lm_backend user = lm_admin password = 1qazxsw2 hostaddr = 127.0.0.1 port = 5432");
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
        for (result::const_iterator c = requests.begin(); c != requests.end(); ++c) {
            request_exits = true;
            target_url_is_included = c[0].as<bool>();
            if(!c[1].is_null()){
                target_url = c[1].as<string>();
            }
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
                while (ss >> word >> count_str){
                    if (word.at(0) == '{'){
                        word = word.substr(2, word.size() - 4);
                    }
                    else{
                        word = word.substr(1, word.size() - 3);
                    }
                    count_str.erase(count_str.size() - 1, 1);
                    count = atoi(count_str.c_str());
                    cout << word << "-" << utf8_strlen(word) << endl;
                    //break;
                }


            }
            break;
        }

        cout << "Operation done successfully" << endl;
        C.disconnect ();

    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        return 1;
    }
}