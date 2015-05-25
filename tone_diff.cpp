#include <iostream>
#include <fstream>
#include <string>

#include "pgsql_connect.h"
#include "mysql_connect.h"
#include "get_parameters.h"

using namespace std;

int main(int argc, char** argv)
{

		string query_string = "";
		ofstream data;

		const char* config_path = "/opt/opinion_miner/general.cfg";
		get_parameters* config = new get_parameters(config_path);
		config->get_general_params();
        config->get_smad_db_params();
		
		mysql_connect* smad_db = new mysql_connect (config->smad_db_host,
                                                    config->smad_db_name,
                                                    config->smad_db_user,
                                                    config->smad_db_pass);

        if (smad_db->connect() == true) {
        	data.open("/opt/opinion_miner/data.csv");
        	query_string = "select tmp_news.emote, tmp_news.temote from tmp_news where tmp_news.emote is not null and tmp_news.temote is not null and tmp_news.date >= '2015-04-01 00:00:00';";
        	smad_db->query(query_string	);
        	while (smad_db->get_result_row() == true) {
        			data << smad_db->get_result_value(0) << ";" << smad_db->get_result_value(1) << endl;
        	}
        	smad_db->delete_result();
        	smad_db->close();
        }
        delete smad_db;
        delete config;
}