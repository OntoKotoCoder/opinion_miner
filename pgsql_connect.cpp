#include <iostream>
#include <cstdlib>
#include <string>

#include "pgsql_connect.h"

using namespace std;

pgsql_connect::pgsql_connect (string new_db_host, string new_db_name,
                              string new_db_user, string new_db_pass,
                              string new_db_encod)
{
	db_params = "host = " + new_db_host +
		    " dbname = " + new_db_name +
		    " user = " + new_db_user +
		    " password = " + new_db_pass +
		    " client_encoding = " + new_db_encod;
	db_host = new_db_host;
	db_name = new_db_name;
}

void pgsql_connect::connect ()
{
	conn = PQconnectdb(&db_params[0]);
	if (PQstatus(conn) != CONNECTION_OK) {
		cout << "PGSQL ERROR: " << PQerrorMessage(conn) << endl;
	}
	else {
		cout << "PGSQL: Connection to '" << db_host << "@" << db_name << "' established" << endl;
	}
}

void pgsql_connect::query (string query_string)
{
	query_error = false;
	query_result = PQexec(conn, &query_string[0]);
	if (PQresultStatus(query_result) != PGRES_TUPLES_OK) {
		query_error = true;
	}
}

string pgsql_connect::get_value (int i, int j)
{
	return string(PQgetvalue(query_result, i, j));	
}

int pgsql_connect::get_int_value (int i, int j)
{
	return atoi(PQgetvalue(query_result, i, j));
}

double pgsql_connect::get_double_value (int i, int j)
{
	return atof(PQgetvalue(query_result, i, j));
}

void pgsql_connect::delete_result ()
{
	PQclear(query_result);
}

void pgsql_connect::close ()
{
	PQfinish(conn);
	cout << "PGSQL: Connection to '" + db_host + "@" + db_name + "' closed" << endl;
}

void pgsql_connect::clear_table (string table_name)
{
	string query_string = "TRUNCATE TABLE " + table_name + ";";
	query_result = PQexec(conn, &query_string[0]);
	
	if (PQresultStatus(query_result) != PGRES_TUPLES_OK) {
		cout << "PGSQL ERROR: " << PQerrorMessage(conn) << endl;	
	}
	else {
		cout << "PGSQL: " << "Из " << db_name << "/" << table_name << " удалены все записи";
	}
}

int pgsql_connect::table_size (string table_name)
{
	string query_string = "SELECT COUNT(*) FROM " + table_name + ";";
	query_result = PQexec(conn, &query_string[0]);

	return atoi(PQgetvalue(query_result, 0, 0));

}
