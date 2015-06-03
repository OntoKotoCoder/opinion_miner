#include <iostream>
#include <cstdlib>
#include <string>

#include <getopt.h>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>

#include "../mysql_connect.h"
#include "../get_parameters.h"

#define HELPER_VERSION "0.2.1"

using namespace std;
using namespace boost;

void print_line(char symbol);

int main(int argc, char** argv)
{
	size_t analyzed_texts = 0;
	
	string config_path = "/opt/opinion_miner/general.cfg";
	string query_string = "";
	
	string text = "";
	string::const_iterator text_start;
    string::const_iterator text_end;

	ofstream input;
	ofstream guids;
	ifstream output;
	stringstream clear_text;
	ofstream clear_data;

	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
    smatch result;
		
	int option;
    int option_index;
    const char* short_options = "c:";
    const struct option long_options[] = {
        {"config", required_argument, nullptr,'c'},
        {nullptr, 0, nullptr, 0}
    };
	while ((option = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch(option)  {
            case 'c': {
				config_path = optarg;
                break;
            }
		}
	}
	
	get_parameters* config = new get_parameters(&config_path[0]);
	config->get_general_params();
	config->get_directories_params();
	config->get_servers_params();	
	mysql_connect* smad_db = new mysql_connect(config);
	
	cout << "TEXT HELPER: v" << HELPER_VERSION << " [Can show the texts by their guid or date!]" << endl;
	cout << "CONFIG PATH: " << config_path << endl;
	cout << "    DB HOST: " << config->smad_db_host << endl;
	cout << "    DB NAME: " << config->smad_db_name << endl;
	
	if (smad_db->connect() == true) {
		// Составляем запрос к базе
		query_string = "select tn.text, tn.guid \
						from tmp_news as tn \
						inner join sources as s on tn.source_id=s.id \
						where (s.type = 'news' or s.type = 'np-news') and tn.category_id = 31;";
		// Отправляем запрос
		smad_db->query(query_string);
		
		// Читаем ответ от базы
		input.open(config->mystem_input);
		guids.open("guids_data");
		while (smad_db->get_result_row() == true) {
			text = smad_db->get_result_value(0);
			text_start = text.begin();
            text_end = text.end();
            // Регуляркой забираем только слова на русском языке и убираем все знаки препинания
			while (u32regex_search(text_start, text_end, result, u32rx)) {
				clear_text << result[1] << " ";
				text_start = result[1].second;
			}	
			input << clear_text.str() << endl;
			clear_text.str("");
			guids << smad_db->get_result_value(1) << endl;
		}
		input.close();
		smad_db->delete_result();
		smad_db->close();
		delete smad_db;
		
		cout << "Start MYSTEM ..." << endl;
		std::system("mystem -cwld /opt/opinion_miner/mystem_data/mystem_input_research /opt/opinion_miner/mystem_data/mystem_output_research");
		
		output.open(config->mystem_output);
		clear_data.open("result_data");
		while (getline(output, text)) {
			text_start = text.begin();
			text_end = text.end();
			while (u32regex_search(text_start, text_end, result, u32rx)) {
				clear_text << result[1] << " ";
				text_start = result[1].second;
            }
			clear_data << clear_text.str() << endl;
			clear_text.str("");
		}	
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
