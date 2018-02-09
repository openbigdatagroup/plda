#include <iostream>
#include <string>
#include <pqxx/pqxx>

#define STATUS_LDA_ANALYSIS 5
#define STATUS_COMPLETE  6

using namespace std;
using namespace pqxx;

int main(int argc, char* argv[]) {
    string const base_topic_req_sql = "SELECT target_url_is_included, target_url from topic_modeling_request "
            "where pk=? and status=?";
    string const base_req_lda_dat_sql = "SELECT * from topic_modeling_lda_data where topic_request=?";
    string const base_req_update_sql = "UPDATE * topic_modeling_request set status = 6 where pk=?";
    string sql;
    int pk = 4;
    unsigned long pos;
    try {
        connection C("dbname = lm_backend user = lm_admin password = 1qazxsw2 \
                             ostaddr = 127.0.0.1 port = 5432");
        if (C.is_open()) {
            cout << "Opened database successfully: " << C.dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
            return 1;
        }
        /* Create SQL statement */
        sql = base_topic_req_sql;
        pos = sql.find('?');
        sql.insert(pos, std::to_string(pk));
        pos = sql.find('?');
        sql.insert(pos, std::to_string(STATUS_LDA_ANALYSIS));

        /* Create a non-transactional object. */
        nontransaction N(C);

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
            return 1;
        }
        cout << "Operation done successfully" << endl;
        C.disconnect ();

        sql = base_req_lda_dat_sql;
        pos = sql.find('?');
        sql.insert(pos, std::to_string(pk));

        result lda_data( N.exec( sql ));

        /* if lda result exists, it means it is completed */
        for (result::const_iterator c = lda_data.begin(); c != lda_data.end(); ++c) {
            work W(C);
            /* Create  SQL UPDATE statement */
            sql = base_req_update_sql;
            pos = sql.find('?');
            sql.insert(pos, std::to_string(pk));
            /* Execute SQL query */
            W.exec( sql );
            W.commit();
            return 1;
        }



    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        return 1;
    }
}