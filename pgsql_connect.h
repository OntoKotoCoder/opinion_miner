#include <string>

#include "postgresql/libpq-fe.h"

using namespace std;

class pgsql_connect
{
private:
	PGconn *conn;
	PGresult *query_result;

	string  db_params,
		db_name,
		db_host;
public:
	bool connect_error;
        bool query_error;

	pgsql_connect ( string new_db_host, string new_db_name,
			string new_db_user, string new_db_pass,
			string new_db_encod );

	void connect ();
	void query (string query_string);
	string get_value (int i, int j);
	int get_int_value (int i, int j);
	double get_double_value (int i, int j);
	void delete_result ();
	void close ();

	// дополнительные функции
	void clear_table (string table_name);
	int table_size (string table_name);
};
