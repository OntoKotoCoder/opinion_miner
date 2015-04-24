#include <iostream>
#include <sstream>
#include <string>

#include "mysql_connect.h"

using namespace std;

mysql_connect::mysql_connect (string new_db_host, string new_db_name,
			      string new_db_user, string new_db_pass)
{
	db_host = new_db_host;
	db_name = new_db_name;
	db_user = new_db_user;
	db_pass = new_db_pass;
}

bool mysql_connect::connect ()
{
	mysql_init(&mysql);
	connection = mysql_real_connect(&mysql, db_host.c_str(), db_user.c_str(), 
					db_pass.c_str(), db_name.c_str(), 3306,0,0);

	if (connection == nullptr) {
		cout << "MySQL ERROR: " << mysql_error(&mysql) << endl;
		return false;
	}
	else {
		cout << "MySQL: Connection to '" << db_host << "@" << db_name << "' established" << endl;
		return true;
	}
}

void mysql_connect::query (string query_string)
{
	query_error = false;
	query_state = mysql_query(connection, query_string.c_str());
	if (query_state !=0) {
		cout << mysql_error(connection) << endl;
		query_error = true;
	}
	result = mysql_store_result(connection);
}

bool mysql_connect::get_result_row ()
{
	row = mysql_fetch_row(result);
	if (row != nullptr)
		return true;
	else
		return false;
}

string mysql_connect::get_result_value (int value_index)
{
	stringstream string_value;
	string_value << row[value_index];
	return string_value.str();
}

void mysql_connect::delete_results ()
{
	mysql_free_result(result);
}

void mysql_connect::close () {
	mysql_close(connection);
	cout << "MySQL: Connection to '" + db_host + "@" + db_name + "' closed" << endl;
}
