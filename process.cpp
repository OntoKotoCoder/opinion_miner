#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <time.h>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "process.h"

using namespace std;
using namespace boost;

process::process ()
{

}

void process::fill_db_with_training_set()
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
		dict_db->set_to_zero("texts_text_id_seq");
		dict_db->delete_result();
		// Забираем из базы данных СМАД'а все тексты с эмоциональной тональностью
		if (config->texts_limit == -1)
			query_string = "select tmp_news.text, tmp_news.myemote, tmp_news.guid from tmp_news where myemote is not null;";
                else
			query_string = "select tmp_news.text, tmp_news.myemote, tmp_news.guid from tmp_news where myemote is not null limit " + to_string(config->texts_limit) + ";";
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
			query_string += "UPDATE texts SET mystem_text = '" 
					+ clear_text.str() + "' WHERE text_id = " + to_string(i) + ";\n";
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

void process::fill_db_with_n_gramms ()
{
	// Забираем параметры из кофнигурационного файла
	const char* config_path = "/opt/sentiment_analysis/general.cfg";
	get_parameters* config = new get_parameters(config_path);
	config->get_general_params();
	config->get_dict_db_params();

	int texts_count = 0;
	int n_gramms_count = 0;
	int current_text_emotion = 0;
	int current_text_id = 0;

	double idf = 0;	
	string query_string;
	string text;
	string n_gramm;

	char_separator<char> sep(" ");
	tokenizer<char_separator<char>>::iterator next_word;
	tokenizer<char_separator<char>>::iterator buff_word;

	unordered_map<string, unordered_map<string, int>> n_gramms;

	pgsql_connect* dict_db = new pgsql_connect (config->dict_db_host,
                                                    config->dict_db_name,
                                                    config->dict_db_user,
                                                    config->dict_db_pass,
                                                    config->dict_db_encod);

	if (dict_db->connect() == true) {
		dict_db->clear_table("dictionary");
                dict_db->set_to_zero("dictionary_n_gramm_id_seq");
		if (config->texts_limit == -1)
			texts_count = dict_db->table_size("texts");
		else
			texts_count = config->texts_limit;
		
		query_string = "SELECT texts.mystem_text, texts.emotion, texts.text_id FROM texts ORDER BY text_id;";
		dict_db->query(query_string);
		for (int i = 0; i < texts_count; i++) {
			text = dict_db->get_value(i, 0);
			current_text_emotion = dict_db->get_int_value(i, 1);
			current_text_id = dict_db->get_int_value(i, 2);

			tokenizer<char_separator<char>> words(text, sep);

			for (next_word = words.begin(); next_word != words.end(); ++next_word) {
				buff_word = next_word;
				for (int j = 1; j <= config->n_gramm_size; j++) {
					n_gramm += *buff_word;
					if (current_text_emotion >= 0) {
						n_gramms[n_gramm]["in_texts_p"]++;
						if (n_gramms[n_gramm]["previous_text_id"] != current_text_id) {
							n_gramms[n_gramm]["texts_with_p"]++;
						}
					}
					else {
						n_gramms[n_gramm]["in_texts_n"]++;
						if (n_gramms[n_gramm]["previous_text_id"] != current_text_id) {
							n_gramms[n_gramm]["texts_with_n"]++;
						}
					}
					n_gramms[n_gramm]["previous_text_id"] = current_text_id;
					n_gramm += " "; ++buff_word;
					if (buff_word == words.end())
						break;
				}
				n_gramm = "";
			}
		}
		dict_db->delete_result();
		query_string = "BEGIN;\n";
		for (auto &this_n_gramm: n_gramms) {
			query_string += "INSERT INTO dictionary (n_gramm, in_texts_n, in_texts_p, texts_with_n, texts_with_p) VALUES ('" +
			this_n_gramm.first + "', " +
			to_string(this_n_gramm.second["in_texts_n"]) + ", " +
			to_string(this_n_gramm.second["in_texts_p"]) + ", " +
			to_string(this_n_gramm.second["texts_with_n"]) + ", " +
			to_string(this_n_gramm.second["texts_with_p"]) + ");\n";
		}
		query_string += "COMMIT;";
		dict_db->query(query_string);
		dict_db->delete_result();

		n_gramms_count = dict_db->table_size("dictionary");			
		query_string = "SELECT dictionary.n_gramm, dictionary.texts_with_p, dictionary.texts_with_n, n_gramm_id FROM dictionary;";
                dict_db->query(query_string);
                query_string = "BEGIN;\n";
                for (int i = 0; i < n_gramms_count; i++) {
                        idf = log((double)texts_count / (dict_db->get_int_value(i, 1) + dict_db->get_int_value(i, 2)));
                        query_string += "UPDATE dictionary set idf = " + to_string(idf) + 
					" WHERE n_gramm_id = " + dict_db->get_value(i, 3) + ";\n";
                }
                query_string += "COMMIT;";
                dict_db->query(query_string);
		dict_db->close();
	}
}

void process::calculate_idf ()
{
	int n_gramms_count = 0;
	int texts_count = 0;
	
	double idf = 0;

	string query_string;
	
	const char* config_path = "/opt/sentiment_analysis/general.cfg";
        get_parameters* config = new get_parameters(config_path);
        config->get_dict_db_params();
	
	pgsql_connect* dict_db = new pgsql_connect (config->dict_db_host,
                                                    config->dict_db_name,
                                                    config->dict_db_user,
                                                    config->dict_db_pass,
                                                    config->dict_db_encod);

	if (dict_db->connect() == true) {
		n_gramms_count = dict_db->table_size("dictionary");
		texts_count = dict_db->table_size("texts");

		query_string = "SELECT dictionary.n_gramm, dictionary.texts_with_p, dictionary.texts_with_n, n_gramm_id FROM dictionary;";
		dict_db->query(query_string);
		query_string = "BEGIN;\n";	
		for (int i = 0; i < n_gramms_count; i++) {
			idf = log((double)texts_count / (dict_db->get_int_value(i, 1) + dict_db->get_int_value(i, 2)));
			query_string += "UPDATE dictionary set idf = " + to_string(idf) + 
					" WHERE n_gramm_id = " + dict_db->get_value(i, 3) + ";\n";
		}
		query_string += "COMMIT;";
		dict_db->query(query_string);
		dict_db->close();
	}
}

void process::calculate_vector_space ()
{
	int n_gramms_count = 0;
	int in_this_text_ngramms = 0;
	int texts_count = 0;
	int n_gramm_id = 0, n_gramm_emotion = 0;
        int text_id = 0;

	double tf = 0, tf_idf = 0;
	double n_gramm_idf = 0;
	double d = 0;			//нормализация
	
	size_t i = 0, j = 0;
	
	clock_t block_time, time;

	string query_string = "";
	string text = "";
	string n_gramm = "";
	
	char_separator<char> sep(" ");

	unordered_multimap<string, unordered_map<string, int>>* texts = new unordered_multimap<string, unordered_map<string, int>>;
	unordered_map<string, int> text_params;
	typedef pair<string, unordered_map<string, int>> new_text;

	unordered_map<string, int> n_gramms;
	
	ofstream vector_space_file;
	
	const char* config_path = "/opt/sentiment_analysis/general.cfg";
        get_parameters* config = new get_parameters(config_path);
        config->get_general_params();
        config->get_dict_db_params();

	pgsql_connect* dict_db = new pgsql_connect (config->dict_db_host,
                                                    config->dict_db_name,
                                                    config->dict_db_user,
                                                    config->dict_db_pass,
                                                    config->dict_db_encod);

	if (dict_db->connect() == true) {
		time = clock();
		n_gramms_count = dict_db->table_size("dictionary");
		if (config->texts_limit == -1)
                        texts_count = dict_db->table_size("texts");
                else
                        texts_count = config->texts_limit;
		
		query_string = "SELECT texts.mystem_text, texts.emotion, texts.text_id FROM texts;";
		dict_db->query(query_string);
		for (size_t i = 0 ; i < texts_count; i++) {
			text = dict_db->get_value(i, 0);
			text_params["text_id"] = dict_db->get_int_value(i, 2);
			text_params["emotion"] = dict_db->get_int_value(i, 1);
			texts->insert(new_text(text, text_params));
		}

		vector_space_file.open(config->v_space_file_name);

		for (auto &this_text: *texts) {
			block_time = clock();
			i++;
			tokenizer<char_separator<char>>* words = new tokenizer<char_separator<char>>(this_text.first, sep);

			query_string = "SELECT dictionary.n_gramm_id, dictionary.idf FROM dictionary WHERE n_gramm IN (";

			// Формируем N-GRAMM'ы
			auto last_word = words->end();
			auto next_word = words->begin();

			do {
				auto buff_word = next_word;
				for (size_t j = 1; j <= config->n_gramm_size; j++) {
					// Формируем очередную N-грамму
					n_gramm += *buff_word;
					in_this_text_ngramms++;
					// Записываем N-грамму в SQL запрос					
					query_string += "'" + n_gramm + "',";
					// Считаем сколько раз встретилась каждая N-грамма
					n_gramms[n_gramm]++;

					n_gramm += " "; ++buff_word;

					if (buff_word == last_word) break;
				}
				n_gramm = ""; ++next_word;
			} while (next_word != words->end());

			// Удалим лишнюю запятую в конце строки
			query_string = query_string.erase(query_string.length() - 1);
			query_string += ") ORDER BY dictionary.n_gramm_id;";
			dict_db->query(query_string);

			j = 0;
			// Найдём коэффициент для нормализации для каждой N-граммы
			for (auto& this_n_gramm: n_gramms) {
				d += pow(dict_db->get_double_value(j, 1), 2);
				j++;
			}
			d = sqrt(d);
			
			if (config->number_of_classes == 2) {
				if (this_text.second["emotion"] >= 0)
					vector_space_file << 1;
				else
					vector_space_file << -1;
			}
			else if (config->number_of_classes == 5) {
				vector_space_file << this_text.second["emotion"];
			}

			j = 0;
			for (auto& this_n_gramm: n_gramms) {
				//----TF----//
				tf = this_n_gramm.second / (double)in_this_text_ngramms;
				//----TF-IDF----//
				if (config->use_tf == true)
					tf_idf = dict_db->get_double_value(j, 1) * tf;
				else
					tf_idf = dict_db->get_double_value(j, 1);
				
				vector_space_file << "\t" << dict_db->get_int_value(j, 0) << ":" << (tf_idf * d);
				j++;
			}
			vector_space_file << "\n";
			
			dict_db->delete_result();
		
			n_gramms.clear();
			delete words;
			in_this_text_ngramms = 0;
			block_time = clock() - block_time;
			cout << "\rProcessed texts: [" << i << "/" << texts_count << "] for: " << (double)block_time * 1000 / CLOCKS_PER_SEC << " ms ";
		}
		time = clock() - time;
		cout << "\nCalculated vector space of: " << (double)time / CLOCKS_PER_SEC << " sec\n";
		vector_space_file.close();
	}
}
void print_progress(char completed_symbol, char not_completed_symbol, int completed_count, int progress_length) 
{
	for (size_t i = 1; i <= completed_count; i++) {
		cout << completed_symbol;
	}
	for (size_t i = completed_count + 1; i <= progress_length; i++) {
		cout << not_completed_symbol;
	}
}
