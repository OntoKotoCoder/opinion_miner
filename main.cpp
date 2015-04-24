#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <getopt.h>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/version.hpp>

#include "main.h"
#include "get_parameters.h"
#include "mysql_connect.h"
#include "pgsql_connect.h"

using namespace std;
using namespace boost;

int main (int argc, char **argv) 
{
	string query_string;
	string text;
	string::const_iterator text_start;
	string::const_iterator text_end;
	stringstream clear_text;
	
	ofstream input;
	ifstream output;
	
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
	smatch result;

	get_parameters* config = new get_parameters("/opt/sentiment_analysis/general.cfg");
	config->get_general_params();
	config->get_smad_db_params();
	
	int option;
	int option_index;
	const char* short_options = "hl:";
	const struct option long_options[] = {
		{"help", no_argument, nullptr, 'h'},
		{"limit", required_argument, nullptr,'l'},
		{nullptr, 0, nullptr, 0}
	};

	while ((option = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch(option) {
			case 'h': {
				get_help();
				break;	
			}
			case 'l': {
				config->texts_limit = atoi(optarg);
				break;		
			}
		}
	}

	cout << "Start programm" << endl;
	get_boost_version();
	mysql_connect* smad_db = new mysql_connect (config->smad_db_host,
						    config->smad_db_name,
						    config->smad_db_user,
						    config->smad_db_pass);
	if (config->texts_limit == -1)
		query_string = "select tmp_news.text, tmp_news.myemote from tmp_news where myemote is not null;";
	else
		query_string = "select tmp_news.text, tmp_news.myemote from tmp_news where myemote is not null limit " + to_string(config->texts_limit) + " ;";

	if (smad_db->connect() == true) {
		smad_db->query(query_string);
		input.open("/opt/sentiment_analysis/input");
		while (smad_db->get_result_row() == true) {
			text = smad_db->get_result_value(0);
			text_start = text.begin();
			text_end = text.end();
			while (u32regex_search(text_start, text_end, result, u32rx)) {
				clear_text << result[1] << " ";
				text_start = result[1].second;		
			}
			input << clear_text.str() << endl << endl;
			clear_text.str("");
		}
		input.close();
		smad_db->delete_results();
		smad_db->close();
		system("mystem -cwld /opt/sentiment_analysis/input /opt/sentiment_analysis/output");
	}
	
	cout << ">> Stem texts:" << endl;
	output.open("/opt/sentiment_analysis/output");
	while (getline(output, text)) {
		if (text != nullptr) {
			text_start = text.begin();
                	text_end = text.end();
			while (u32regex_search(text_start, text_end, result, u32rx)) {
				clear_text << result[1] << " ";
				text_start = result[1].second;
			}
			cout << clear_text.str() << endl << endl;
			clear_text.str("");
		}
	}

	cout << "Test PostgreSQL connection" << endl;
	config->get_dict_db_params();
	print_line ('=');
	pgsql_connect* dict_db = new pgsql_connect (config->dict_db_host,
                                                    config->dict_db_name,
                                                    config->dict_db_user,
                                                    config->dict_db_pass,
						    config->dict_db_encod);
	dict_db->connect();
	dict_db->query("select version();");
	cout << "Query result:" << endl;
	cout << dict_db->get_value(0, 0) << endl;
	dict_db->delete_result();
	dict_db->close();	
}

void print_line (int symbols_count, char symbol) 
{
	for (size_t i = 0; i < symbols_count; i ++) {
		cout << symbol;
	}
	cout << endl;
}
void print_line (char symbol)
{
	int symbols_count = atoi(getenv("COLUMNS"));
	for (size_t i = 0; i < symbols_count; i ++) {
                cout << symbol;
        }
        cout << endl;
}
void get_boost_version ()
{
	std::cout << "Boost version: "     
		  << BOOST_VERSION / 100000     << "."  // major version
		  << BOOST_VERSION / 100 % 1000 << "."  // minior version
		  << BOOST_VERSION % 100                // patch level
		  << std::endl;
}
void get_help ()
{
	cout << "Sentiment analysis:" << endl;
	print_line('=');
	cout << " -l : определяет кол-во текстов забираемых из базы данных СМАД" << endl
	     << " -h : выводит информацию о программе" << endl
	     << " конфигурационный файл: /opt/sentiment_analysis/general.cfg" << endl;
	print_line('=');
}
