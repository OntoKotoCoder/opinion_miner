#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>		
#include <ctime>

#include "postgresql/libpq-fe.h"	//для работы с PostgreSQL

#include "get_parameters.h"

using namespace std;

class pgsql_connect
{
private:
	PGconn 		*connection;
	PGresult 	*query_result;

	string  db_params,
			db_name,
			db_host;

	ofstream query_log;
	ofstream worker_log;

public:	
	bool connect_error;
	bool query_error;

	pgsql_connect (get_parameters* config);
	~pgsql_connect ();
	bool connect ();
	void query (string query_string);
	unsigned int rows_count ();
	string get_value (int i, int j);
	int get_int_value (int i, int j);
	double get_double_value (int i, int j);
	void delete_result ();
	void close ();

	// дополнительные функции
	void clear_table (string table_name);
	void set_to_zero (string sequence_name);
	int table_size (string table_name);
	string last_date (string table_name, string column_name);

private:
	char* get_time();
};
