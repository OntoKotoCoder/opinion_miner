#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include <mysql/mysql.h>

#include "get_parameters.h"

using namespace std;

class mysql_connect
{
private:
	MYSQL 		*connection, mysql;
	MYSQL_RES	*result;
	MYSQL_ROW	row;

	int query_state;
	
	string	db_host,
			db_name,
			db_user,
			db_pass;

	ofstream query_log;
	ofstream worker_log;

public:
	bool query_error;

	mysql_connect (get_parameters* config);
	bool connect ();
	void query (string query_string);
	bool get_result_row ();
	string get_result_value (int value_ind);
	void delete_result ();
	void close ();
private:
	char* get_time();
};
