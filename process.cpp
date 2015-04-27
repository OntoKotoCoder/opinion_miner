#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>

#include "process.h"

using namespace std;
using namespace boost;

process::process ()
{

}

void process::get_texts_with_emotion ()
{
	size_t i = 0;
        string text, smad_guid, emotion;
        string::const_iterator text_start;
        string::const_iterator text_end;
        stringstream clear_text;
	string query_string = "";
        
	ofstream input;
        ifstream output;
	
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
        smatch result;

	// Забираем параметры из кофнигурационного файла	
	const char* config_path = "/opt/sentiment_analysis/general.cfg";
        get_parameters* config = new get_parameters(config_path);
        config->get_general_params();
        config->get_smad_db_params();
	config->get_dict_db_params();
	
	// Создаём подключение к базам	
	mysql_connect* smad_db = new mysql_connect (config->smad_db_host,
                                                    config->smad_db_name,
                                                    config->smad_db_user,
                                                    config->smad_db_pass);
        
	pgsql_connect* dict_db = new pgsql_connect (config->dict_db_host,
                                                    config->dict_db_name,
                                                    config->dict_db_user,
                                                    config->dict_db_pass,
                                                    config->dict_db_encod);

	if (smad_db->connect() == true && dict_db->connect() == true) {
		// Очищаем таблицу с текстами
		dict_db->clear_table("texts");
		dict_db->delete_result();
		// Забираем из базы данных СМАД'а все тексты с эмоциональной тональностью
		query_string = "select tmp_news.text, tmp_news.myemote, tmp_news.guid from tmp_news where myemote is not null;";
                smad_db->query(query_string);
                input.open("/opt/sentiment_analysis/input");	// сюда запишем исходные тексты

		// Читаем результаты запроса к базe данных СМАД'а и преобразовываем тексты
                while (smad_db->get_result_row() == true) {
			i++;
                        text = smad_db->get_result_value(0);
			emotion = smad_db->get_result_value(1);
			smad_guid = smad_db->get_result_value(2);
                        text_start = text.begin();
                        text_end = text.end();
			
			// Регуляркой забираем только слова на русском языке и убираем все знаки препинания
                        while (u32regex_search(text_start, text_end, result, u32rx)) {
                                clear_text << result[1] << " ";
                                text_start = result[1].second;
                        }
			// В файл input заносим обработанный текст
                        input << clear_text.str() << endl;
			query_string = "INSERT INTO texts (original_text, pretreat_text, emotion, smad_guid) VALUES ('" 
					+ text + "', '" + clear_text.str() + "', " + emotion + ", '" + smad_guid + "');";
                        dict_db->query(query_string);
			dict_db->delete_result();
			clear_text.str("");
			cout << "\rPorcessed texts: " << i;
                }
		cout << endl;
                input.close();
		smad_db->delete_result();
		// Больше база данных СМАД'а нам не нужна, отключаемся
                smad_db->close();
                
		// Прогоняем тексты из input через mystem, результаты пишем output 
		cout << "Mystem: start mysteming texts" << endl;
		system("mystem -cwld /opt/sentiment_analysis/input /opt/sentiment_analysis/output");
		cout << "Mystem: finished mysteming texts" << endl;
	
		// Читаем output и заносим обработанные mystem'ом тексты в базу
		output.open("/opt/sentiment_analysis/output");
		query_string = "BEGIN;\n";
		i = 0;
		while (getline(output, text)) {
			i++;
			text_start = text.begin();
                        text_end = text.end();
			// Регуляркой убераем {} оставленные mystem'ом
			while (u32regex_search(text_start, text_end, result, u32rx)) {
                                clear_text << result[1] << " ";
                                text_start = result[1].second;
                        }
			query_string += "INSERT INTO texts (mystem_text) VALUES ('" 
					+ clear_text.str() + "') WHERE text_id = " + to_string(i) + ";\n";
			clear_text.str("");
			cout << "\rInserted mystem texts to database: " << i;
		}
		cout << endl;
		// Отправлем в базу сразу несколько запросов через BEGIN -> COMMIT
		query_string += "COMMIT;";
		dict_db->query(query_string);
		dict_db->delete_result();
		// Закончиили формирования таблицы texts
		dict_db->close();
        }
}
