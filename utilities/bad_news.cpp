#include <iostream>
#include <cstdlib>
#include <string>

#include <getopt.h>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/timer/timer.hpp>

#include "../mysql_connect.h"
#include "../get_parameters.h"

#define HELPER_VERSION "0.2.1"

using namespace std;
using namespace boost;
using namespace boost::timer;
using namespace boost::chrono;

void print_line(char symbol);

int main(int argc, char** argv)
{
	bool from_file = false;
	bool by_date = false;
	bool by_guid = false;

	size_t analyzed_texts = 0;
	size_t empty_texts = 0;
	size_t english_texts = 0;

	string config_path = "/opt/opinion_miner/general.cfg";
	string query_string = "";
	string guid = "";
	string guid_file_name = "";
	
	string text = "";
	string::const_iterator text_start;
    string::const_iterator text_end;
	stringstream rus_text;

	ifstream guid_file;

	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
	smatch result;	
	
	cpu_timer timer;
	nanosecond_type oneSecond(1000000000LL);
	typedef duration<double> sec;

	int option;
    int option_index;
    const char* short_options = "c:g:d:f:";
    const struct option long_options[] = {
        {"config", required_argument, nullptr,'c'},
        {"guid", required_argument, nullptr,'g'},
        {"date", required_argument, nullptr,'d'},
        {"from_file", required_argument, nullptr,'f'},
        {nullptr, 0, nullptr, 0}
    };
	while ((option = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch(option)  {
            case 'c': {
				config_path = optarg;
                break;
            }
            case 'g': {
                guid = optarg;
				by_guid = true;
				break;
            }
            case 'd': {
                guid = optarg;
				by_date = true;
				break;
            }
			case 'f': {
				from_file = true;
				guid_file_name = optarg;
			}
		}
	}
	if (by_guid == true and by_date == true) {
		cout << "ERROR: Use one of the two keys! [-g OR -d]" << endl;
		exit(0);
	}
	
	get_parameters* config = new get_parameters(&config_path[0]);
	config->get_general_params();
	config->get_servers_params();	
	
	mysql_connect* smad_db = new mysql_connect(config);
	
	cout << "TEXT HELPER: v" << HELPER_VERSION << " [Can show the texts by their guid or date!]" << endl;
	cout << "CONFIG PATH: " << config_path << endl;
	cout << "    DB HOST: " << config->smad_db_host << endl;
	cout << "    DB NAME: " << config->smad_db_name << endl;
	
	if (smad_db->connect() == true) {
		// Составляем запрос к базе
		query_string = "select tmp_news.guid, \
								tmp_news.text_id, \
								tmp_news.date, \
								tmp_news.emote, \
								tmp_news.myemote, \
        						tmp_news.temote, \
        						tmp_news.title, \
        						tmp_news.text \
						from tmp_news ";
		if (by_guid == true) {
			query_string += " where tmp_news.guid ";
		}
		else if (by_date == true) {
			query_string += " where tmp_news.date ";
		}
		else {
			query_string += " where tmp_news.guid ";	// Что бы запрос не ломался
		}
		if (from_file == true) {
			guid_file.open(guid_file_name);
			query_string += "in (";
			while (getline(guid_file, guid)) {
				query_string += "'" + guid + "',";
			}
			query_string = query_string.erase(query_string.length() - 1);
			query_string += ");";
		}
		else {
			query_string += "= '" + guid + "';";
		}
		// Отправляем запрос
		smad_db->query(query_string);
		
		// Читаем ответ от базы
		while (smad_db->get_result_row() == true) {
			text = smad_db->get_result_value(7);
			text_start = text.begin();
			text_end = text.end();
			while (u32regex_search(text_start, text_end, result, u32rx)) {
                rus_text << result[1] << " ";
                text_start = result[1].second;
            }
			print_line('=');
			cout << "GUID: " << smad_db->get_result_value(0) << " | " <<
					"TEXT_ID: " << smad_db->get_result_value(1) << " | " <<
					"DATE: " << smad_db->get_result_value(2) << " | " <<
					"EMOTE: " << smad_db->get_result_value(3) << " | "  <<
					"M_EMOTE: " << smad_db->get_result_value(4) << " | " <<
					"T_EMOTE: " << smad_db->get_result_value(5)  << endl;
			print_line('=');
			cout << "[TITLE:]" << endl;
			print_line('-');
			cout << smad_db->get_result_value(6) << endl << endl;
			cout << "[TEXT:]" << endl;
			print_line('-');
			cout << text << endl << endl;
			
			if (text == "") {
				empty_texts++;
			}
			if (text != "" and rus_text.str() == "") {
				english_texts++;
			}
			analyzed_texts++;
			
			rus_text.str("");
			text = "";
		}

		smad_db->delete_result();
		smad_db->close();
		delete smad_db;
		//timer.start();
		//sec seconds = nanoseconds(timer.elapsed().wall);
		cout << "Results of the analysis:" << endl <<
				"------------------------" << endl;
		cout << "Texts processed: " << analyzed_texts << endl;
		cout << "  English texts: " << english_texts << endl;
		cout << "    Empty texts: " << empty_texts << endl;
	}
	cout << endl;
	delete config;
}

void print_line (char symbol)
{
    size_t symbols_count = atoi(getenv("COLUMNS"));
    for (size_t i = 0; i < symbols_count; i ++) {
                cout << symbol;
        }
        cout << endl;
}
