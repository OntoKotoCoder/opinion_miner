#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>
//#include <ctime>
//#include <time.h>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "process.h"

using namespace std;
using namespace boost;

static int(*info)(const char *fmt, ...) = &printf;
int console_width = atoi(getenv("COLUMNS"));
unsigned int progress_length = 30;

process::process ()
{
	worker_log.open("/var/log/opinion_miner/worker.log", fstream::app);
	error_log.open("/var/log/opinion_miner/error.log", fstream::app);
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
			query_string = "select tmp_news.text, tmp_news.myemote, tmp_news.guid tmp_date.guid from tmp_news where myemote is not null;";
                else
			query_string = "select tmp_news.text, tmp_news.myemote, tmp_news.guid from tmp_news where myemote is not null limit " + to_string(config->texts_limit) + ";";
		smad_db->query(query_string);
                input.open("/opt/opinion_miner/input");	// сюда запишем исходные тексты

		// Читаем результаты запроса к базe данных СМАД'а и преобразовываем тексты
		system("tput civis");
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
		system("tput cnorm");

		cout << endl;
                input.close();
		smad_db->delete_result();
		// Больше база данных СМАД'а нам не нужна, отключаемся
                smad_db->close();
                
		// Прогоняем тексты из input через mystem, результаты пишем output 
		cout << "Mystem: start mysteming texts" << endl;
		system("mystem -cwld /opt/opinion_miner/input /opt/opinion_miner/output");
		cout << "Mystem: finished mysteming texts" << endl;
	
		// Читаем output и заносим обработанные mystem'ом тексты в базу
		output.open("/opt/opinion_miner/output");
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

void process::update_db_with_training_set()
{
	size_t i = 0;
	size_t dictionary_size = 0;
	string last_date;
        string text, smad_guid, emotion, date;
        string::const_iterator text_start;
        string::const_iterator text_end;
        stringstream clear_text;
	string query_string = "";
        
	ofstream input;
        ifstream output;
	
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
        smatch result;

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
		dictionary_size = dict_db->table_size("texts");
		last_date = dict_db->last_date("texts", "date");
		
		// Забираем из базы данных СМАД'а все тексты с эмоциональной тональностью
		if (config->texts_limit == -1)
			query_string = "select tmp_news.text, tmp_news.myemote, tmp_news.guid, tmp_news.date from tmp_news where myemote is not null and date > '" + last_date + "';";
                else
			query_string = "select tmp_news.text, tmp_news.myemote, tmp_news.guid, tmp_news.date from tmp_news where myemote is not null and date > '" + last_date + "' limit " + to_string(config->texts_limit) + ";";
		smad_db->query(query_string);
                input.open("/opt/opinion_miner/input");	// сюда запишем исходные тексты

		// Читаем результаты запроса к базe данных СМАД'а и преобразовываем тексты
		system("tput civis");
                while (smad_db->get_result_row() == true) {
			i++;
                        text = smad_db->get_result_value(0);
			emotion = smad_db->get_result_value(1);
			smad_guid = smad_db->get_result_value(2);
			date = smad_db->get_result_value(3);
                        text_start = text.begin();
                        text_end = text.end();
			
			// Регуляркой забираем только слова на русском языке и убираем все знаки препинания
                        while (u32regex_search(text_start, text_end, result, u32rx)) {
                                clear_text << result[1] << " ";
                                text_start = result[1].second;
                        }
			// В файл input заносим обработанный текст
                        input << clear_text.str() << endl;
			query_string = "INSERT INTO texts (original_text, pretreat_text, emotion, smad_guid, date) VALUES ('" 
					+ text + "', '" + clear_text.str() + "', " + emotion + ", '" + smad_guid + "', '" + date + "');";
                        dict_db->query(query_string);
			dict_db->delete_result();
			clear_text.str("");
			cout << "\rPorcessed texts: " << i;
                }
		system("tput cnorm");

		cout << endl;
                input.close();
		smad_db->delete_result();
		// Больше база данных СМАД'а нам не нужна, отключаемся
                smad_db->close();
            
		// Прогоняем тексты из input через mystem, результаты пишем output 
		cout << "Mystem: start mysteming texts" << endl;
		system("mystem -cwld /opt/opinion_miner/input /opt/opinion_miner/output");
		cout << "Mystem: finished mysteming texts" << endl;
	
		// Читаем output и заносим обработанные mystem'ом тексты в базу
		output.open("/opt/opinion_miner/output");
		query_string = "BEGIN;\n";
		i = dictionary_size;
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

void process::fill_db_with_training_set_from_file()
{
	bool is_text = false,
	     is_emotion = false,
	     is_date = false;
	
	size_t i = 0;
	string some_data;
        string text, smad_guid, emotion, date = "";
        string::const_iterator text_start;
        string::const_iterator text_end;
        stringstream clear_text;
	string query_string = "";
        
	ifstream texts_file;
	ofstream input;
        ifstream output;
	
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
	u32regex u32_kill_control = make_u32regex("([\\x00-\\x1F])");
        smatch result;

	// Создаём подключение к базам	
	pgsql_connect* dict_db = new pgsql_connect (config->dict_db_host,
                                                    config->dict_db_name,
                                                    config->dict_db_user,
                                                    config->dict_db_pass,
                                                    config->dict_db_encod);
	
	texts_file.open("/opt/opinion_miner/data/texts");       // отсюда заберём тексты
	cout << "Read texts from file: /opt/opinion_miner/data/texts" << endl;
	
	if (dict_db->connect() == true && texts_file.is_open()) {
		// Очищаем таблицу с текстами
		dict_db->clear_table("texts");
		dict_db->set_to_zero("texts_text_id_seq");
		dict_db->delete_result();
                
		input.open("/opt/opinion_miner/input");		// сюда запишем исходные тексты
		// Читаем результаты запроса к базe данных СМАД'а и преобразовываем тексты
		system("tput civis");
                while (getline(texts_file, some_data)) {
			if (some_data.length() == 5) {
				some_data = some_data.erase(some_data.length() - 1);
			}
			if (some_data == "text") {
				is_date = false;
				is_text = true;
			}
			else if (some_data == "emot") {
				is_text = false;
				is_emotion = true;
			}
			else if (some_data == "date") {
				is_emotion = false;
				is_date = true;
			}
			else {
				if (is_text == true) {
					text += some_data + "\n";
				}
				else if (is_emotion == true) {
					emotion = some_data;
				}
				else if (is_date == true) {
					date = some_data;
				}
			}
			if (is_date == true && some_data != "date") {
				i++;
                        	text_start = text.begin();
                        	text_end = text.end();
				if (date == "") {
					cout << "ATTENTION! " << i << endl;
				}
				// Регуляркой забираем только слова на русском языке и убираем все знаки препинания
                        	while (u32regex_search(text_start, text_end, result, u32rx)) {
                                	clear_text << result[1] << " ";
                                	text_start = result[1].second;
                        	}
				// В файл input заносим обработанный текст
				input << clear_text.str() << endl;
				//========================
				// не забыть про smad_guid
				//========================
				smad_guid = "NULL";
				query_string = "INSERT INTO texts (original_text, pretreat_text, emotion, date, smad_guid) VALUES ('" 
						+ text + "', '" + clear_text.str() + "', " + emotion + ", '" + date + "', '" + smad_guid + "');";
                        	dict_db->query(query_string);
				dict_db->delete_result();
				clear_text.str("");
				text = "";
				cout << "\rPorcessed texts: " << i;
			}
                }
		system("tput cnorm");

		cout << endl;
                input.close();
                
		// Прогоняем тексты из input через mystem, результаты пишем output 
		cout << "Mystem: start mysteming texts" << endl;
		system("mystem -cwld /opt/opinion_miner/input /opt/opinion_miner/output");
		cout << "Mystem: finished mysteming texts" << endl;
	
		// Читаем output и заносим обработанные mystem'ом тексты в базу
		output.open("/opt/opinion_miner/output");
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
	size_t i = 0;
	size_t limit = 0;
	unsigned int texts_count = 0;
	unsigned int n_gramms_count = 0;
	int current_text_emotion = 0;
	int current_text_id = 0;

	double idf = 0;	
	string query_string;
	string text;
	string n_gramm;
	
	vector<string> query_strings;
	char_separator<char> sep(" ");
	tokenizer<char_separator<char>>* words(string, char_separator<char>);
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
		for (size_t i = 0; i < texts_count; i++) {
			text = dict_db->get_value(i, 0);
			current_text_emotion = dict_db->get_int_value(i, 1);
			current_text_id = dict_db->get_int_value(i, 2);

			tokenizer<char_separator<char>> words(text, sep);

			for (next_word = words.begin(); next_word != words.end(); ++next_word) {
				buff_word = next_word;
				for (size_t j = 1; j <= config->n_gramm_size; j++) {
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
		i = 0;
		for (auto &this_n_gramm: n_gramms) {
			i++;
			query_string += "INSERT INTO dictionary (n_gramm, in_texts_n, in_texts_p, texts_with_n, texts_with_p) VALUES ('" +
			this_n_gramm.first + "', " +
			to_string(this_n_gramm.second["in_texts_n"]) + ", " +
			to_string(this_n_gramm.second["in_texts_p"]) + ", " +
			to_string(this_n_gramm.second["texts_with_n"]) + ", " +
			to_string(this_n_gramm.second["texts_with_p"]) + ");\n";
			if (i == 50000) {
				query_string += "COMMIT;";
				query_strings.push_back(query_string);
				query_string = "BEGIN;\n";
				i = 0;
			}
		}
		query_string += "COMMIT;";
		query_strings.push_back(query_string);
		query_string = "";

		for (auto &this_query_string: query_strings) {
			dict_db->query(this_query_string);
		}
		dict_db->delete_result();
		query_strings.clear();		

		n_gramms_count = dict_db->table_size("dictionary");			
		query_string = "SELECT dictionary.n_gramm, dictionary.texts_with_p, dictionary.texts_with_n, n_gramm_id FROM dictionary;";
                dict_db->query(query_string);
                
		query_string = "BEGIN;\n";
		limit = 50000;
                for (size_t i = 0; i < n_gramms_count; i++) {
                        idf = log((double)texts_count / (dict_db->get_int_value(i, 1) + dict_db->get_int_value(i, 2)));
                        query_string += "UPDATE dictionary set idf = " + to_string(idf) + 
					" WHERE n_gramm_id = " + dict_db->get_value(i, 3) + ";\n";
			if (i == limit) {
				query_string += "COMMIT;";
                                query_strings.push_back(query_string);
                                query_string = "BEGIN;\n";
                                limit = limit + 50000;
			}
                }
                query_string += "COMMIT;";
		query_strings.push_back(query_string);
		query_string = "";
		dict_db->delete_result();

		for (auto &this_query_string: query_strings) {
                        dict_db->query(this_query_string);
                }
                dict_db->delete_result();
		query_strings.clear();
		dict_db->close();
	}
}

void process::calculate_idf ()
{
	int n_gramms_count = 0;
	int texts_count = 0;
	
	double idf = 0;

	string query_string;
	
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
	unsigned int n_gramms_count = 0,
		     in_this_text_ngramms = 0,
		     texts_count = 0;

	double tf = 0, tf_idf = 0;
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

		cout << "N-gramms count:\t" << n_gramms_count << endl;
		cout << "   Texts count:\t" << texts_count << endl;	
		
		query_string = "SELECT texts.mystem_text, texts.emotion, texts.text_id FROM texts;";
		dict_db->query(query_string);
		for (size_t i = 0 ; i < texts_count; i++) {
			text = dict_db->get_value(i, 0);
			text_params["text_id"] = dict_db->get_int_value(i, 2);
			text_params["emotion"] = dict_db->get_int_value(i, 1);
			texts->insert(new_text(text, text_params));
		}

		vector_space_file.open(config->v_space_file_name);
		
		// Отключим курсор что бы не моргвл
		system("tput civis");
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
				in_this_text_ngramms++;
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
		system("tput cnorm");
		
		time = clock() - time;
		cout << "\nCalculated vector space of: " << (double)time / CLOCKS_PER_SEC << " sec\n";
		vector_space_file.close();
	}
}

void process::calculate_vector_space_from_smad_texts ()
{
	unsigned int in_this_text_ngramms = 0,
		     texts_count = 0;

	double tf = 0, tf_idf = 0;
	double d = 0;			//нормализация
	
	size_t i = 0;
	size_t error = 0;
	
	string last_date = "";
	string query_string = "";
	string text = "";
	string n_gramm = "";

	string::const_iterator text_start;
        string::const_iterator text_end;
	
	stringstream clear_text;

	char_separator<char> sep(" ");

	//unordered_map<unsigned int, unordered_map<string, string>> texts = new unordered_map<unsigned int, unordered_map<string, string>>;
	unordered_map<unsigned int, unordered_map<string, string>> texts;
        unordered_map<string, string> text_params;
        typedef pair<unsigned int, unordered_map<string, string>> new_text;

	unordered_map<string, int> n_gramms;
	
	fstream last_date_file;
	ofstream input;
	ifstream output;
	ofstream vector_space_file;
	
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
        smatch result;	
	
	mysql_connect* smad_db = new mysql_connect (config->smad_db_host,
                                                    config->smad_db_name,
                                                    config->smad_db_user,
                                                    config->smad_db_pass);

	pgsql_connect* dict_db = new pgsql_connect (config->dict_db_host,
                                                    config->dict_db_name,
                                                    config->dict_db_user,
                                                    config->dict_db_pass,
                                                    config->dict_db_encod);

	last_date_file.open("/opt/opinion_miner/last.date");
	getline(last_date_file, last_date);
	worker_log << get_time() << " [ WORKER] # Find the latest date: " << last_date << endl; // Потом удалить!
	last_date_file.close();
	
	if (smad_db->connect() == true && dict_db->connect() == true) {
		// Добавь 'myemote is not null and ' для тестирования на обучающей выборке
		if (config->texts_limit == -1) {
			query_string = "select tmp_news.text, tmp_news.guid, tmp_news.date from tmp_news where date >= '" + last_date + "';";
			worker_log << get_time() << " [ WORKER] # Select all texts from SMAD database" << endl;
		}
                else {
			query_string = "select tmp_news.text, tmp_news.guid, tmp_news.date from tmp_news where date >= '" + last_date + "' limit " + to_string(config->texts_limit) + ";";
			worker_log << get_time() << " [ WORKER] # Select " << config->texts_limit << " texts from SMAD database" << endl;
		}
		smad_db->query(query_string);
		input.open("/opt/opinion_miner/input");
		while (smad_db->get_result_row() == true) {
			i++;
			text = smad_db->get_result_value(0);
			text_start = text.begin();
                        text_end = text.end();
			// Регуляркой забираем только слова на русском языке и убираем все знаки препинания
                        while (u32regex_search(text_start, text_end, result, u32rx)) {
                                clear_text << result[1] << " ";
                                text_start = result[1].second;
                        }
			input << clear_text.str() << endl;
                        texts[i]["guid"] = smad_db->get_result_value(1);
                        texts[i]["date"] = smad_db->get_result_value(2);
                        //texts.insert(new_text(i, text_params));
			last_date = smad_db->get_result_value(2);
			clear_text.str("");
		}
		input.close();
		// БД СМАД'а нам больше не понадобится отключаемся
		smad_db->delete_result();
		smad_db->close();

		// Прогоняем тексты из input через mystem, результаты пишем output
		system("mystem -cwld /opt/opinion_miner/input /opt/opinion_miner/output");
		vector_space_file.open(config->v_space_file_name);
		output.open("/opt/opinion_miner/output");
		worker_log << get_time() << " [ WORKER] # Processed by using <mystem> " << i << " texts" << endl;
	
		worker_log << get_time() << " [ WORKER] # Create a vector space: ";	
		i = 0;
		//for (auto &this_text: *texts) {
		while (getline(output, text)) {
			i++;
			//getline(output, text);
			text_start = text.begin();
                        text_end = text.end();
			while (u32regex_search(text_start, text_end, result, u32rx)) {
                                clear_text << result[1] << " ";
                                text_start = result[1].second;
                        }
			text = clear_text.str();
			clear_text.str("");
			if (text != "") {
			tokenizer<char_separator<char>>* words = new tokenizer<char_separator<char>>(text, sep);

			query_string = "SELECT dictionary.n_gramm_id, dictionary.idf, dictionary.n_gramm FROM dictionary WHERE n_gramm IN (";

			// Формируем N-GRAMM'ы
			auto last_word = words->end();
			auto next_word = words->begin();

			do {
				auto buff_word = next_word;
				for (size_t j = 1; j <= config->n_gramm_size; j++) {
					// Формируем очередную N-грамму
					n_gramm += *buff_word;
					// Записываем N-грамму в SQL запрос					
					query_string += "'" + n_gramm + "',";
					// Считаем сколько раз встретилась каждая N-грамма
					n_gramms[n_gramm]++;

					n_gramm += " "; ++buff_word;

					if (buff_word == last_word) break;
				}
				n_gramm = ""; ++next_word;
			} while (next_word != words->end());
			delete words;
			
			// Удалим лишнюю запятую в конце строки
			query_string = query_string.erase(query_string.length() - 1);
			query_string += ") ORDER BY dictionary.n_gramm_id;";
			dict_db->query(query_string);

			// Найдём коэффициент для нормализации для каждой N-граммы
			for (size_t i = 0; i < dict_db->rows_count(); i++) {
				in_this_text_ngramms++;
				d += pow(dict_db->get_double_value(i, 1), 2);
			}	
			d = sqrt(d);
			
			// Записываем guid в начале каждого вектора
			vector_space_file << texts[i]["guid"];	

			//j = 0;
			for (size_t i = 0; i < dict_db->rows_count(); i++) {
				//----TF----//
				tf = n_gramms[dict_db->get_value(i, 2)] / (double)in_this_text_ngramms;
				//----TF-IDF----//
				if (config->use_tf == true)
                                        tf_idf = dict_db->get_double_value(i, 1) * tf;
                                else
                                        tf_idf = dict_db->get_double_value(i, 1);

                                vector_space_file << "\t" << dict_db->get_int_value(i, 0) << ":" << (tf_idf * d);
			}
			vector_space_file << endl;
			n_gramms.clear();
                        dict_db->delete_result();
                        in_this_text_ngramms = 0;
			}
			else {
				error++;
				error_log << get_time() << " Error in text with guid = " <<  texts[i]["guid"] << endl;
			}
		}
		output.close();
		vector_space_file.close();
		worker_log << "finished " << i << " texts with error " << error << endl;
		// Записываем новую дату в last.date
		last_date_file.open("/opt/opinion_miner/last.date");
		last_date_file << last_date;
		last_date_file.close();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//					SVM functions					//
//////////////////////////////////////////////////////////////////////////////////////////
void process::start_svm_train()
{
	const char *error_msg;

	get_svm_parameters();

	cout << "Read vector space data file";
	read_v_space_file(config->v_space_file_name);
	cout << "\n";

	error_msg = svm_check_parameter(&prob, &param);

	if (error_msg) fprintf(stderr, "ERROR: %s\n", error_msg);
	else {
		cout << "Train SVM classifier";
		model = svm_train(&prob, &param);

		if (svm_save_model(&config->model_file_name[0], model))
			fprintf(stderr, "can't save model to file %s\n", &config->model_file_name[0]);
		svm_free_and_destroy_model(&model);
	}

	svm_destroy_param(&param);
	free(prob.y);
	free(prob.x);
	free(v_space);
}

void process::start_svm_predict()
{
	char result_file_name[100];
	strcpy(result_file_name, "/opt/opinion_miner/data/result.data_");
	model = svm_load_model(&config->model_file_name[0]);

	v_space = new svm_node[max_nr_attr];
		
	svm_check_probability_model(model);
	predict(&config->v_space_file_name[0], strcat(result_file_name, get_time()));
	svm_free_and_destroy_model(&model);
	free(v_space);
}

void process::start_calc_emotion()
{
	size_t i = 0;
	string query_string = "";
	mysql_connect* smad_db = new mysql_connect (config->smad_db_host,
                                                    config->smad_db_name,
                                                    config->smad_db_user,
                                                    config->smad_db_pass);

	calculate_vector_space_from_smad_texts();	
	model = svm_load_model(&config->model_file_name[0]);
	v_space = new svm_node[max_nr_attr];
	svm_check_probability_model(model);
	predict_to_query(&config->v_space_file_name[0]);
	if (smad_db->connect() == true) {
		worker_log << get_time() << " [ WORKER] # Rated: ";
		smad_db->query("select count(tmp_news.guid) from tmp_news where temote is not null;");
		smad_db->get_result_row();
		worker_log << smad_db->get_result_value(0) << " from ";
		smad_db->delete_result();
		smad_db->query("select count(tmp_news.guid) from tmp_news;");
		smad_db->get_result_row();
		worker_log << smad_db->get_result_value(0) << " texts" << endl;
		smad_db->delete_result();
		smad_db->close();
	}
	svm_free_and_destroy_model(&model);
	free(v_space);
}

void process::read_v_space_file(string v_space_file_name)
{
	int max_index, inst_max_index, i;
	size_t elements, j;

	char *endptr;
	char *idx, *val, *label;

	string line;
	ifstream v_space_file;
	v_space_file.open(v_space_file_name);

	if (!v_space_file.is_open())
		printf("can't open input file %s\n", &v_space_file_name[0]);
	else {
		prob.l = 0;
		elements = 0;

		while (getline(v_space_file, line)) {
				
			char *p = strtok(&line[0], " \t");

			while (true) {
				p = strtok(NULL, " \t");
				if (p == NULL || *p == '\n') break;
				++elements;
			}
			++elements;
			++prob.l;
		}

		v_space_file.close(); v_space_file.open(v_space_file_name);

		prob.y = new double[prob.l];
		prob.x = new struct svm_node*[prob.l];
		v_space = new struct svm_node[elements];

		max_index = 0;
		j = 0;
		
		// Отключим курсор что бы не моргвл
		system("tput civis");
		for (i = 0; i < prob.l; i++) {
			inst_max_index = -1;
			getline(v_space_file, line);

			prob.x[i] = &v_space[j];
			label = strtok(&line[0], " \t\n");
			if (label == NULL) exit_input_error(i + 1);

			prob.y[i] = strtod(label, &endptr);
			if (endptr == label || *endptr != '\0') exit_input_error(i + 1);

			while (true) {
				idx = strtok(NULL, ":");
				val = strtok(NULL, " \t");

				if (val == NULL) break;

				errno = 0;
				v_space[j].index = (int)strtol(idx, &endptr, 10);

				if (endptr == idx || errno != 0 || *endptr != '\0' || v_space[j].index <= inst_max_index)
					exit_input_error(i + 1);
				else
					inst_max_index = v_space[j].index;	// Наибольший индекс в конкретном векторе

				errno = 0;
				v_space[j].value = strtod(val, &endptr);
				if (endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
					exit_input_error(i + 1);
				++j;
			}
			if (inst_max_index > max_index)
				max_index = inst_max_index;	// Наибольший индекс во всей коллекции векторов
			v_space[j++].index = -1;

			printf("\rProcessed vectros from data file: %d/%d", (i + 1), prob.l);
		}
		system("tput cnorm");
		cout << endl << "Write model to file: " << config->model_file_name << endl;
		cout << "Class count : " << config->number_of_classes << endl;
		if (param.gamma == 0 && max_index > 0)
			param.gamma = 1.0 / max_index;

		v_space_file.close();
	}
}

void process::predict(char* v_space_file_name, char* result_file_name)
{
	int correct = 0;
	int total = 0;

	int wrong_positive = 0, wrong_negative = 0, wrong_0_positive = 0, wrong_0_negative = 0;
	int wrong_1_positive = 0, wrong_1_negative = 0, wrong_2_positive = 0, wrong_2_negative = 0;
	
	double TP = 0, FP = 0, FN = 0, TN = 0;
	double P_positive = 0, P_negative = 0, P_average = 0;
	double R_positive = 0, R_negative = 0, R_average = 0;
	double F_positive = 0, F_negative = 0, F_on_average = 0, F_average = 0;
	double A;

	double error = 0;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;

	size_t j = 0;

	fstream v_space_file;
	ofstream result_file;
	ofstream results;
	string line;
	v_space_file.open(v_space_file_name);
	
	result_file.open(result_file_name);

	cout << "Get vector space from file: " << v_space_file_name << endl;
	cout << "       Get model from file: " << config->model_file_name << endl;
	cout << "     Write results to file: " << result_file_name << endl;

	size_t texts_count = 0;
	while (getline(v_space_file, line)) texts_count++;

	// переоткроем файл с векторами
	v_space_file.close(); v_space_file.open(v_space_file_name);

	double one_element = (double)progress_length / (double)texts_count;
	// Выключим курсор что бы не моргал во время отрисовки прогресс бара 	
	system("tput civis");
	
	cout << "\nProcessed texts:\n";
	while (getline(v_space_file, line)) {
		j++;
		
		int i = 0;
		double target_label, predict_label;
		char *idx, *val, *label, *endptr;
		int inst_max_index = -1;	// strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0
		
		// Забираем первый символ в строке - номер класса
		label = strtok(&line[0], " \t\n");

		// Проверим не пустая ли строка нам попалась
		if (label == nullptr)
			exit_input_error(total + 1);
		
		// Конвертируем в double номер класса 
		target_label = strtod(label, &endptr);
		
		// Проверим 
		if (endptr == label || *endptr != '\0')
			exit_input_error(total + 1);

		while (true) {
			if (i >= max_nr_attr - 1) {
				max_nr_attr *= 2;
				v_space = (struct svm_node *) realloc(v_space, max_nr_attr*sizeof(struct svm_node));
			}

			idx = strtok(nullptr, ":");	// Вытаскиваем очередной индекс признака из вектора
			val = strtok(nullptr, " \t");	// Вытаскиваем очередное значение признака из вектора

			if (val == nullptr) break;	// Конец вектора

			errno = 0;
			// Конвертируем в long int индекс и записываем в структуру v_space
			v_space[i].index = (int)strtol(idx, &endptr, 10);
			if (endptr == idx || errno != 0 || *endptr != '\0' || v_space[i].index <= inst_max_index)
				exit_input_error(total + 1);
			else
				inst_max_index = v_space[i].index;

			errno = 0;
			// Конвертируем в double значние и записываем в структуру v_space
			v_space[i].value = strtod(val, &endptr);
			if (endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
				exit_input_error(total + 1);

			++i;
		}

		printf("\r%5d/%-5d ", j, texts_count);
		cout << "["; print_progress('*', '_', j * one_element, progress_length); cout << "]";
		v_space[i].index = -1;

		predict_label = svm_predict(model, v_space);
		result_file << predict_label << endl;

		if (config->number_of_classes == 5) {
			if (predict_label == target_label && target_label >= 0) {
				++wrong_0_positive;
				++correct;
			}
			else if (predict_label == target_label && target_label < 0) {
				++wrong_0_negative;
				++correct;
			}
			else if (predict_label < 0 && target_label >= 0)
				++wrong_positive;
			else if (predict_label >= 0 && target_label < 0)
				++wrong_negative;
			else if (predict_label >= 0 && target_label >= 0 && abs(predict_label - target_label) == 1)
				++wrong_1_positive;
			else if (predict_label < 0 && target_label < 0 && abs(predict_label - target_label) == 1)
				++wrong_1_negative;
			else if (predict_label >= 0 && target_label >= 0 && abs(predict_label - target_label) == 2)
				++wrong_2_positive;
			else if (predict_label >= 0 && target_label >= 0 && abs(predict_label - target_label) == 2)
				++wrong_2_negative;
		}
		else if (config->number_of_classes == 2) {
			if (predict_label == target_label)
				++correct;
			if (predict_label == target_label && target_label >= 0)
				++TP;
			else if (predict_label != target_label && target_label >= 0)
				++FP;
			else if (predict_label == target_label && target_label < 0)
				++TN;
			else if (predict_label != target_label && target_label < 0)
				++FN;
		}
	
		error += (predict_label - target_label)*(predict_label - target_label);
		sump += predict_label;
		sumt += target_label;	
		sumpp += predict_label*predict_label;
		sumtt += target_label*target_label;
		sumpt += predict_label*target_label;
		++total;
	}
	system("tput cnorm");
	results.open("results.csv");

	info("\nAccuracy = %g%% (%d/%d) (classification)\n", (double)correct / total * 100, correct, total);
	if (config->number_of_classes == 5) {
		cout << "\nDefine negative instead positive: " << wrong_positive;
		results << "Define negative instead positive;" << wrong_positive << endl;
		cout << "\nDefine positive instead negative: " << wrong_negative;
		results << "Define positive instead negative;" << wrong_negative << endl;
		cout << "\n   Wrong for 0 class on positive: " << wrong_0_positive;
		results << "Wrong for 0 class on positive;" << wrong_0_positive << endl;
		cout << "\n   Wrong for 0 class on negative: " << wrong_0_negative;
		results << "Wrong for 0 class on negative;" << wrong_0_negative << endl;
		cout << "\n   Wrong for 1 class on positive: " << wrong_1_positive;	
		results << "Wrong for 1 class on positive;" << wrong_1_positive << endl;
		cout << "\n   Wrong for 1 class on negative: " << wrong_1_negative;
		results << "Wrong for 1 class on negative;" << wrong_1_negative << endl;
		cout << "\n   Wrong for 2 class on positive: " << wrong_2_positive;
		results << "Wrong for 2 class on positive;" << wrong_2_positive << endl;
		cout << "\n   Wrong for 2 class on negative: " << wrong_2_negative << endl;
		results << "Wrong for 2 class on negative;" << wrong_2_negative << endl;
	}
	else if (config->number_of_classes == 2) {
		P_positive = TP / (TP + FP);
		P_negative = TN / (TN + FN);
		P_average = (P_positive + P_negative) / 2;
		R_positive = TP / (TP + FN);
		R_negative = TN / (TN + FP);
		R_average = (R_positive + R_negative) / 2;
		F_positive = (2 * P_positive * R_positive) / (P_positive + R_positive);
		F_negative = (2 * P_negative * R_negative) / (P_negative + R_negative);
		F_on_average = (2 * P_average * R_average) / (P_average + R_average);
		F_average = (F_positive + F_negative) / 2;
		A = (TP + TN) / (TP + FN + FP + TN);
		cout << "Results:\n";
		cout << "---------------------------";
		cout << "\nPrecision positive: " << P_positive; results << "Precision positive;" << P_positive << endl;
		cout << "\nPrecision negative: " << P_negative; results << "Precision negative;" << P_negative << endl;
		cout << "\n Precision average: " << P_average; results << "Precision average;" << P_average << endl;
		cout << "\n   Recall positive: " << R_positive; results << "Recall positive;" << R_positive << endl;
		cout << "\n   Recall negative: " << R_negative; results << "Recall negative;" << R_negative << endl;
		cout << "\n    Recall average: " << R_average; results << "Recall average;" << R_average << endl;
		cout << "\n  Funct 1 positive: " << F_positive; results << "Function 1 positive;" << F_positive << endl;
		cout << "\n  Funct 1 negative: " << F_negative; results << "Function 1 negative;" << F_negative << endl;
		cout << "\nFunct 1 on average: " << F_on_average; results << "Function 1 on average;" << F_on_average << endl;
		cout << "\n   Funct 1 average: " << F_average; results << "Function 1 average;" << F_average << endl;
		cout << "\n          Accuracy: " << A << endl; results << "Accuracy;" << A;
	}
	results.close();
	result_file.close();	
}

void process::predict_to_query(char* v_space_file_name)
{
	int total = 0;
	fstream v_space_file;

	string line = "";
	string query_string = "";
	
	v_space_file.open(v_space_file_name);

	size_t texts_count = 0;

	mysql_connect* smad_db = new mysql_connect (config->smad_db_host,
                                                    config->smad_db_name,
                                                    config->smad_db_user,
                                                    config->smad_db_pass);

	// Считаем сколько текстов в файле векторного пространства
	while (getline(v_space_file, line)) texts_count++;

	// переоткроем файл с векторами
	v_space_file.close(); v_space_file.open(v_space_file_name);

	worker_log << get_time() << " [ WORKER] # Calculate of emotional tonality: ";
	if (smad_db->connect() == true) {	
		while (getline(v_space_file, line)) {
			int i = 0;
			int 	predict_label;
			char *idx, *val, *guid, *endptr;
			int inst_max_index = -1;	// strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0
			
			// Забираем из начала строки guid
			guid = strtok(&line[0], " \t\n");

			// Проверим не пустая ли строка нам попалась
			if (guid == nullptr)
				exit_input_error(total + 1);
			
			while (true) {
				if (i >= max_nr_attr - 1) {
					max_nr_attr *= 2;
					v_space = (struct svm_node *) realloc(v_space, max_nr_attr*sizeof(struct svm_node));
				}
				
				idx = strtok(nullptr, ":");	// Вытаскиваем очередной индекс признака из вектора
				val = strtok(nullptr, " \t");	// Вытаскиваем очередное значение признака из вектора
				if (val == nullptr) break;	// Конец вектора
				errno = 0;
				// Конвертируем в long int индекс и записываем в структуру v_space
				v_space[i].index = (int)strtol(idx, &endptr, 10);
				if (endptr == idx || errno != 0 || *endptr != '\0' || v_space[i].index <= inst_max_index)
					exit_input_error(total + 1);
				else
					inst_max_index = v_space[i].index;

				errno = 0;
				// Конвертируем в double значние и записываем в структуру v_space
				v_space[i].value = strtod(val, &endptr);
				if (endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr)))
					exit_input_error(total + 1);

				++i;
			}
			v_space[i].index = -1;
			// Получаем значение эмоциональной тональности для данного текста
			predict_label = svm_predict(model, v_space);
			query_string = "update tmp_news set tmp_news.temote = " + to_string(predict_label) + " where tmp_news.guid = '" + string(guid) + "';";
			smad_db->query(query_string);
		}
	}
	worker_log << "finished with error 0" << endl;
	v_space_file.close();
}

void process::get_svm_parameters() 
{
	param.svm_type = config->svm_type;
	param.kernel_type = config->kernel_type;
	param.degree = 3;
	param.gamma = 0;
	param.coef0 = 0;
	param.nu = config->nu;
	param.cache_size = 100;
	param.C = 1000;
	param.eps = 1e-3;
	param.p = 0.1;
	param.shrinking = 1;
	param.probability = 0;
	param.nr_weight = 0;
	param.weight_label = NULL;
	param.weight = NULL;
	//cross_validation = 0;
}

void process::exit_input_error(int line_num)
{
	fprintf(stderr, "Wrong input format at line %d\n", line_num);
	exit(1);
}

void process::print_progress(char completed_symbol, char not_completed_symbol, unsigned int completed_count, unsigned int progress_length) 
{
	for (size_t i = 1; i <= completed_count + 1; i++) {
		cout << completed_symbol;
	}
	for (size_t i = completed_count + 2; i <= progress_length; i++) {
		cout << not_completed_symbol;
	}
}
void process::print_line (size_t symbols_count, char symbol)
{
        for (size_t i = 0; i < symbols_count; i ++) {
                cout << symbol;
        }
        cout << endl;
}
void process::print_line (char symbol)
{
        size_t symbols_count = atoi(getenv("COLUMNS"));
        for (size_t i = 0; i < symbols_count; i ++) {
                cout << symbol;
        }
        cout << endl;
}
char* process::get_time ()
{
	time_t rawtime;
        struct tm* timeinfo;
        char* _buffer = new char[17];

        time (&rawtime);
        timeinfo = localtime (&rawtime);

        strftime (_buffer,80,"%d.%m.%y-%H:%M:%S",timeinfo);
	return _buffer;
}
