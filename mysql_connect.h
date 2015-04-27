#include <string>

#include <mysql/mysql.h>

using namespace std;

class mysql_connect
{
private:
	MYSQL *connection, mysql;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int query_state;
	
	string   db_host,
		 db_name,
		 db_user,
		 db_pass;
public:
	bool query_error;

	mysql_connect ( string new_db_host,
			string new_db_name,
			string new_db_user,
			string new_db_pass );
	bool connect ();
	void query (string query_string);
	bool get_result_row ();
	string get_result_value (int value_ind);
	void delete_result ();
	void close ();
};
