#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <ctime>

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
	
	query_log.open("/var/log/opinion_miner/query.log", fstream::app);
	worker_log.open("/var/log/opinion_miner/worker.log", fstream::app);
}

bool pgsql_connect::connect ()
{
	conn = PQconnectdb(&db_params[0]);
	if (PQstatus(conn) != CONNECTION_OK) {
		worker_log << get_time() << " [  PGSQL] # ERROR: " << PQerrorMessage(conn) << endl;
		return false;
	}
	else {
		worker_log << get_time() << " [  PGSQL] # Connection to '" << db_host << "@" << db_name << "' established" << endl;
		return true;
	}
}

void pgsql_connect::query (string query_string)
{
	query_error = false;
	query_result = PQexec(conn, &query_string[0]);

	stringstream error_message;
        error_message << PQerrorMessage(conn);

	if (error_message.str() != "") {
		query_error = true;
		query_log << get_time() << " " << error_message.str() << endl;
	}
}

unsigned int pgsql_connect::rows_count()
{
	return PQntuples(query_result);
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
	worker_log << get_time() << " [  PGSQL] # Connection to '" + db_host + "@" + db_name + "' closed" << endl;
}

void pgsql_connect::clear_table (string table_name)
{
	string query_string = "TRUNCATE TABLE " + table_name + ";";
	query_result = PQexec(conn, &query_string[0]);
	
	stringstream error_message;
	error_message << PQerrorMessage(conn);	
	
	if (error_message.str() != "") {
		cout << " [  PGSQL] " << error_message.str();	
	}
	else {
		cout << " [  PGSQL] " << "Removed from '" << db_name << "/" << table_name << "' all entries" << endl;
	}
}

void pgsql_connect::set_to_zero (string sequence_name)
{
	string query_string = "SELECT pg_catalog.setval('" + sequence_name +"', 0, true);";
	query_result = PQexec(conn, &query_string[0]);

	stringstream error_message;
        error_message << PQerrorMessage(conn);

        if (error_message.str() != "") {
                cout << " [  PGSQL] " << error_message.str();
        }
        else {
                cout << " [  PGSQL] " << "Sequence '" << sequence_name << "' set by 1" << endl;
        }
}

int pgsql_connect::table_size (string table_name)
{
	int size = 0;
	string query_string = "SELECT COUNT(*) FROM " + table_name + ";";
	query_result = PQexec(conn, &query_string[0]);

	size = atoi(PQgetvalue(query_result, 0, 0));
	PQclear(query_result);
	
	return size;
}

string pgsql_connect::last_date (string table_name, string column_name)
{
	string query_string = "SELECT " + table_name + "." + column_name + " from " + table_name  + " order by " + column_name  + " desc limit 1;";
	stringstream error_message;
	string date = "";
	
	query_result = PQexec(conn, &query_string[0]);
	error_message << PQerrorMessage(conn);

	if (error_message.str() != "") {
		query_log << get_time() << " " << error_message.str() << endl;
        }
	else {
		date = string(PQgetvalue(query_result, 0, 0));
	}

	return date;
}

char* pgsql_connect::get_time ()
{
        time_t rawtime;
        struct tm* timeinfo;
        char* _buffer = new char[17];

        time (&rawtime);
        timeinfo = localtime (&rawtime);

        strftime (_buffer,80,"%d.%m.%y-%H:%M:%S",timeinfo);
        return _buffer;
}
