#include "mysql_connect.h"

mysql_connect::mysql_connect (get_parameters* config)
{
	db_host = config->smad_db_host;
	db_name = config->smad_db_name;
	db_user = config->smad_db_user;
	db_pass = config->smad_db_pass;

	query_log.open(config->query_log, fstream::app);
	worker_log.open(config->worker_log, fstream::app);
}

bool mysql_connect::connect ()
{
	mysql_init(&mysql);
	connection = mysql_real_connect(&mysql, db_host.c_str(), db_user.c_str(), 
					db_pass.c_str(), db_name.c_str(), 3306,0,0);

	if (connection == nullptr) {
		worker_log << get_time() << " [  MySQL] # ERROR: " << mysql_error(&mysql) << endl;
		return false;
	}
	else {
		worker_log << get_time() << " [  MySQL] # Connection to '" << db_host << "@" << db_name << "' established" << endl;
		return true;
	}
}

void mysql_connect::query (string query_string)
{
	query_error = false;
	query_state = mysql_query(connection, query_string.c_str());
	if (query_state !=0) {
		cout << get_time() << " " << mysql_error(connection) << endl;
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

void mysql_connect::delete_result ()
{
	mysql_free_result(result);
}

void mysql_connect::close () {
	mysql_close(connection);
	worker_log << get_time() << " [  MySQL] # Connection to '" + db_host + "@" + db_name + "' closed" << endl;
}

char* mysql_connect::get_time ()
{
        time_t rawtime;
        struct tm* timeinfo;
        char* _buffer = new char[17];

        time (&rawtime);
        timeinfo = localtime (&rawtime);

        strftime (_buffer,80,"%d.%m.%y-%H:%M:%S",timeinfo);
        return _buffer;
}
