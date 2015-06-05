#include "processd.h"

processd::processd (get_parameters* config)
{
	worker_log.open(config->worker_log, fstream::app);
	error_log.open(config->error_log, fstream::app);
	processd::config = config;
}

processd::~processd ()
{
	worker_log.close();
	error_log.close();
}

int processd::calculate_tonality ()
{
	if (create_vector_space(get_texts()) == 1) {
		model = svm_load_model(&config->model_file_name[0]);
		//v_space = new svm_node[max_nr_attr];
		svm_check_probability_model(model);
		send_tonality(define_tonality(config->vector_space_file_name));
		svm_free_and_destroy_model(&model);
    	free(v_space);
	}
	return 1;
}

// Функция забирает из базы данных SMAD тексты, прогоняет через mystem
// На выходе файл: mystem_output с обработанными текстами и список guid'ов текстов
new_guids processd::get_texts ()
{
	size_t i = 0;
	string query_string = "";
	string last_date = "";

	string text = "";
	stringstream clear_text;
	string::const_iterator text_start;
	string::const_iterator text_end;
	new_guids guids;

	fstream last_date_file;
	ofstream mystem_input;

	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
	smatch result;

	mysql_connect* smad_db = new mysql_connect (config);

	// Забираем из файла /path/to/prog/last.date дату. По ней будем выбирать тексты из базы
	last_date_file.open(config->last_date);
	getline(last_date_file, last_date);
	worker_log << get_time() << " [ WORKER] # Find the latest date: " << last_date << endl; // Потом удалить!
	last_date_file.close();

	if (smad_db->connect() == true) {
		// Формируем запрос - выбираем из базы n-ое количество текстов
		if (config->texts_limit == -1) {
			query_string = "select tmp_news.text, tmp_news.guid, tmp_news.date \
							from tmp_news \
							where date >= '" + last_date + "' and tmp_news.temote is null;";
			worker_log << get_time() << " [ WORKER] # Select all texts from SMAD database" << endl;
		}
		else {
			query_string = "select tmp_news.text, tmp_news.guid, tmp_news.date \
							from tmp_news \
							where tmp_news.date >= '" + last_date + "' and tmp_news.temote is null \
							limit " + to_string(config->texts_limit) + ";";
			worker_log << get_time() << " [ WORKER] # Select " << config->texts_limit << " texts from SMAD database" << endl;
		}
		// Отправляем запрос
		smad_db->query(query_string);

		// Читаем ответ - записываем тексты в /path/to/prog/mystem_data/input
		mystem_input.open(config->mystem_input);
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
			mystem_input << clear_text.str() << endl;
			guids[i] = smad_db->get_result_value(1);
			last_date = smad_db->get_result_value(2);
			clear_text.str("");
		}
		mystem_input.close();
		// БД СМАД'а нам больше не понадобится отключаемся
		smad_db->delete_result();
		smad_db->close();
		delete smad_db;

		// Прогоняем тексты из input через mystem, результаты пишем output
		std::system("mystem -cwld /opt/opinion_miner/mystem_data/mystem_input /opt/opinion_miner/mystem_data/mystem_output");
		worker_log << get_time() << " [ WORKER] # Processed by using <mystem> " << i << " texts" << endl;

		// Записываем новую дату в last.date
		last_date_file.open(config->last_date);
		last_date_file << last_date;
		last_date_file.close();		
	}

	// Возвращаем список guid'ов
	return guids;
}

int processd::create_vector_space (new_guids guids)
{
	unsigned int in_this_text_ngramms = 0;

	double 	tf 		= 0,
			tf_idf 	= 0,
			d 		= 0;			//нормализация
	
	size_t i = 0;
	size_t error = 0;
	
	string query_string = "";
	
	string text = "";
	stringstream clear_text;
	string::const_iterator text_start;
	string::const_iterator text_end;
	string n_gramm = "";

	char_separator<char> sep(" ");

	unordered_map<string, int> n_gramms;
	
	ifstream mystem_output;
	ofstream vector_space_file;
	
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
	smatch result;	

	pgsql_connect* dict_db = new pgsql_connect (config);
	
	// Поехали
	if (dict_db->connect() == true) {
		// Создаём векторное пространство
		vector_space_file.open(config->vector_space_file_name);
		mystem_output.open(config->mystem_output);
		worker_log << get_time() << " [ WORKER] # Create a vector space: ";	
		i = 0;
		while (getline(mystem_output, text)) {
			i++;
			text_start = text.begin();
			text_end = text.end();
			// Убираем из текста скобки - "{" и "}"
			while (u32regex_search(text_start, text_end, result, u32rx)) {
				clear_text << result[1] << " ";
				text_start = result[1].second;
			}

			if (text != "" and clear_text.str() != "") {
				text = clear_text.str();
				tokenizer<char_separator<char>>* words = new tokenizer<char_separator<char>>(text, sep);
				// Формируем запрос к словарю n-gramm - забираем те n-gramm'ы, 
				// которые встретились в конкретном тексте
				query_string = "SELECT dictionary.n_gramm_id, dictionary.idf, dictionary.n_gramm \
								FROM dictionary \
								WHERE n_gramm IN (";

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
				// n-gramm'ы сформированы, очищаем words
				delete words;
			
				// Удалим лишнюю запятую в конце строки
				query_string = query_string.erase(query_string.length() - 1);
				query_string += ") ORDER BY dictionary.n_gramm_id;";

				// Запрос сформирован, отправляем
				dict_db->query(query_string);

				// Найдём коэффициент для нормализации для каждой N-граммы
				for (size_t i = 0; i < dict_db->rows_count(); i++) {
					in_this_text_ngramms++;
					d += pow(dict_db->get_double_value(i, 1), 2);
				}
				d = sqrt(d);
			
				// Записываем guid и кол-во n-gramm в начале каждого вектора
				vector_space_file << guids[i] << ":" << in_this_text_ngramms;	

				for (size_t i = 0; i < dict_db->rows_count(); i++) {
					// Рассчитываем TF
					tf = n_gramms[dict_db->get_value(i, 2)] / (double)in_this_text_ngramms;
					// Рассчитываем TF-IDF
					if (config->use_tf == true)
						tf_idf = dict_db->get_double_value(i, 1) * tf;
					else
						tf_idf = dict_db->get_double_value(i, 1);

					vector_space_file << "\t" << dict_db->get_int_value(i, 0) << ":" << (tf_idf * d);
				}
				vector_space_file << endl;
				// Очищаем контейнер с n-gramm'ами и результат выполнения запроса
				n_gramms.clear();
				dict_db->delete_result();
				in_this_text_ngramms = 0;
			}
			else if (text == "") {
				// Если же нам встретился пустой текст, то делаем пометку об этом в error_log
				error++;
				error_log << get_time() << " Error [empty text] in text with guid = " <<  guids[i] << endl;
			}
			else if (clear_text.str() == "") {
				// Если же нам встретился текст на английском языке, то делаем пометку об этом в error_log
				error++;
				error_log << get_time() << " Error [english text] in text with guid = " <<  guids[i] << endl;
			}
			// Очищаем поток с отформатированным текстом
			clear_text.str("");
		}
		worker_log << "finished " << i << " texts with error " << error << endl;
		mystem_output.close();
		vector_space_file.close();
		dict_db->close();	
		delete dict_db;
	}
	return 1;
}

new_emotions processd::define_tonality (string vector_space_file_name)
{
	size_t line_num = 1;

	string line = "";

	new_emotions emotions;

	fstream vector_space_file;
	vector_space_file.open(vector_space_file_name);

	// Рассчитываем тональность 
	worker_log << get_time() << " [ WORKER] # Calculate of emotional tonality: ";
	if (vector_space_file.is_open()) {	
		while (getline(vector_space_file, line)) {
			size_t i = 0;
			int predict_label;
			char *idx, *val, *guid, *elements, *endptr;
			int inst_max_index = -1;
			
			// Забираем из начала строки кол-во n-gramm и guid
			guid = strtok(&line[0], ":");
			elements = strtok(nullptr, " \t\n");

			v_space = new struct svm_node[(int)strtol(elements, &endptr, 10) + 1];

			// Проверим не пустая ли строка нам попалась
			if (guid == nullptr)
				error_log << get_time() << " Error in file: '" + vector_space_file_name + "' in line: " <<  line_num << endl;
			
			while (true) {
				/*if (i >= max_nr_attr - 1) {
					max_nr_attr *= 2;
					v_space = (struct svm_node *) realloc(v_space, max_nr_attr*sizeof(struct svm_node));
				}*/
				idx = strtok(nullptr, ":");	// Вытаскиваем очередной индекс признака из вектора
				val = strtok(nullptr, " \t");	// Вытаскиваем очередное значение признака из вектора
				if (val == nullptr) break;	// Конец вектора
				errno = 0;
				// Конвертируем в long int индекс и записываем в структуру v_space
				v_space[i].index = (int)strtol(idx, &endptr, 10);
				if (endptr != idx || errno == 0 || *endptr == '\0' || v_space[i].index > inst_max_index) {
					inst_max_index = v_space[i].index;
				}
				else {
					error_log << get_time() << " Error in file: '" + vector_space_file_name + "' in line: " <<  line_num << endl;
				}
				errno = 0;
				// Конвертируем в double значние и записываем в структуру v_space
				v_space[i].value = strtod(val, &endptr);
				if (endptr == val || errno != 0 || (*endptr != '\0' && !isspace(*endptr))) {
					error_log << get_time() << " Error in file: '" + vector_space_file_name + "' in line: " <<  line_num << endl;
				}
				++i;
				line_num++;
			}
			v_space[i].index = -1;
			// Получаем значение эмоциональной тональности для данного текста
			predict_label = svm_predict(model, v_space);
			emotions[string(guid)] = predict_label;
		}
	}
	worker_log << "finished with error 0" << endl;
	return emotions;
}

int processd::send_tonality(new_emotions emotions)
{
	size_t i = 0;
	string query_string = "";

	mysql_connect* smad_db = new mysql_connect(config);
	if (smad_db->connect() == true) {
		query_string = "update tmp_news set temote = case ";
		for (auto &emotion: emotions) {
			i ++;
			query_string += "when guid = '" + emotion.first + "' then " + to_string(emotion.second) + " ";
		}
		query_string += "end where guid in (";
		for (auto &emotion: emotions) {
			query_string += "'" + emotion.first + "',";
		}
		query_string = query_string.erase(query_string.length() - 1);
		query_string += ");";
		if (i > 0) {
			smad_db->query(query_string);
			worker_log << get_time() << " [ WORKER] # Sending " + to_string(i) + " values of emotional tone to SMAD database" << endl;
		}
		else {
			worker_log << get_time() << " [ WORKER] # No ratings of emotional tone and nothing to send to SMAD database" << endl;
		}
		smad_db->close();
	}
	delete smad_db;
	return 1;
}

char* processd::get_time ()
{
	time_t rawtime;
	struct tm* timeinfo;
	char* _buffer = new char[17];

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime (_buffer,80,"%d.%m.%y-%H:%M:%S",timeinfo);
	return _buffer;
}