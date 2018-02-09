#include <iostream>
#include <string>
#include <pqxx/pqxx>

#define STATUS_LDA_ANALYSIS 5
#define STATUS_COMPLETE  6

using namespace std;
using namespace pqxx;

int main(int argc, char* argv[]) {
    string const base_topic_req_sql = "SELECT target_url_is_included, target_url from topic_modeling_request "
            "where id =  and status = ";
    string const base_req_lda_data_sql = "SELECT * from topic_modeling_lda_data where topic_request = ";
    string const base_req_update_sql = "UPDATE * topic_modeling_request set status = 6 where id = ";
    string const base_page_data_sql = "SELECT word_dict from topic_modeling_url_page_data where topic_request = ";
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
        pos = sql.find('=') + 2;
        sql.insert(pos, std::to_string(pk));
        pos = sql.find('=', pos + 1) + 2;
        sql.insert(pos, std::to_string(STATUS_LDA_ANALYSIS));

        /* Execute SQL query */
        result requests( N.exec( sql ));

        /* List down all the records */
        bool request_exits = false;
        bool target_url_is_included;
        string target_url;
        for (result::const_iterator c = requests.begin(); c != requests.end(); ++c) {
            request_exits = true;
            target_url_is_included = c[0].as<bool>();
            target_url = c[1].as<string>();
            break;
        }

        if (!request_exits){
            C.disconnect();
            cout << "uncompleted request " << pk << " does not exist" << endl;
            return 1;
        }

        sql = base_req_lda_data_sql;
        pos = sql.find('?') + 2;
        sql.insert(pos, std::to_string(pk));
        result lda_data( N.exec( sql ));

        /* Create a transactional object. */
        work W(C);

        /* if lda result exists, it means it is completed */
        for (result::const_iterator c = lda_data.begin(); c != lda_data.end(); ++c) {
            /* Create  SQL UPDATE statement */
            sql = base_req_update_sql;
            pos = sql.find('=') + 2;
            sql.insert(pos, std::to_string(pk));
            /* Execute SQL query */
            W.exec( sql );
            W.commit();
            C.disconnect ();
            return 1;
        }

        sql = base_page_data_sql;
        pos = sql.find('=') + 2;
        sql.insert(pos, std::to_string(pk));
        result page_data( N.exec( sql ));
        for (result::const_iterator c = page_data.begin(); c != page_data.end(); ++c) {

        }

        cout << "Operation done successfully" << endl;
        C.disconnect ();

    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        return 1;
    }
}