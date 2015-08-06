#include "trend_process.h"

trend_process::trend_process (get_parameters* config)
{
	trend_process::config = config;
}

trend_process::~trend_process ()
{

}

void trend_process::start_fill_db_with_words (string name, bool with_title, bool with_text, int data_input)
{
	trend_process::with_title = with_title;
	trend_process::with_text = with_text;
	if (data_input == FROM_DB) {
		if (fill_db_with_words(get_texts_from_category(name)) == 1) {
			cout << "Finished with status [  OK  ]!" << endl;
		}
		else {
			cout << "Finished with status [FAILED]!" << endl;
		}	
	}
	else if (data_input == FROM_FILE) {
		get_texts_from_file(name);
		/*if (fill_db_with_words(get_texts_from_file(name)) == 1) {
			cout << "Finished with status [  OK  ]!" << endl;
		}
		else {
			cout << "Finished with status [FAILED]!" << endl;
		}*/
	}
}

void trend_process::start_clustering (int clusters_count)
{
	string query_string = "";
	vector<string> query_strings;

	unordered_map<int, list<double>> data;
	list<double> data_params;
	unordered_map<int, int> result_clusters;

	k_means* do_k_means = new k_means(config);
	pgsql_connect* dict_db = new pgsql_connect (config, TO_COUT);
	
	if (dict_db->connect() == true) {
		cout << "Choose the data clustering\t\t\t\t\t";
		query_string = "select cd.word_id, cd.frequency \
						from category_dictionary as cd;";
		dict_db->query(query_string);
		for (size_t i = 0; i < dict_db->rows_count(); i++) {
			data_params.push_back(dict_db->get_double_value(i, 1));
			data[dict_db->get_int_value(i, 0)] = data_params;
			data_params.clear();
		}
		dict_db->delete_result();
		cout << "[OK]" << endl;

		cout << "Begin to clustering ->" << endl;
		result_clusters = do_k_means->clustering(data, clusters_count, MANHATTAN);

		cout << "Submitting the results of the clustering database\t\t";
		size_t i = 0;
		query_string = "BEGIN;\n";
		for (auto &unit: result_clusters) {
			i++;
			query_string += "UPDATE category_dictionary SET cluster_id = " + to_string(unit.second)
							+ " WHERE word_id = " + to_string(unit.first) + ";\n";
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
		query_strings.clear();
		cout << "[OK]" << endl;

		dict_db->close();
		delete dict_db;
	}
}

category trend_process::get_texts_from_category (string category_name)
{
	size_t i = 0;

	string query_string = "";
	string text = "";
	stringstream clear_text;
	string::const_iterator text_start;
	string::const_iterator text_end;

	category category_param;
	category_param.name = category_name;
	category_param.id = -1;

	ofstream mystem_input;
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)");
	smatch result;

	mysql_connect* smad_db = new mysql_connect (config);

	if (smad_db->connect() == true) {
		// Забираем из базы тексты относящиеся к категории - category_name из источников - news и np-news (новости)
		cout << "Select the text from the category '" << category_name << "'"; cout << "\t";
		query_string = "select tn.title, tn.text, sc.category_id \
						from tmp_news as tn \
						inner join subcategorys as sc on tn.category_id=sc.category_id \
						inner join sources as s on tn.source_id=s.id \
						where (s.type = 'news' or s.type = 'np-news') and sc.name = '" + category_name + "';";
		
		// Отправляем запрос
		smad_db->query(query_string);
		cout << "\t[OK]" << endl;

		// Читаем ответ - записываем тексты в /path/to/prog/mystem_data/mystem_input
		mystem_input.open(config->mystem_input);
		while (smad_db->get_result_row() == true) {
			i++;
			// Записываем в возвращаемый umap<,> имя катеогрии и id
			if (category_param.id == -1) {
				category_param.id = stoi(smad_db->get_result_value(2));
			}
			text = "";
			// Обрабатываем очередной текст
			if (with_title == true) {
				text += smad_db->get_result_value(0) + " ";
			}
			if (with_text == true) {
				text += smad_db->get_result_value(1) + " ";
			}
			text_start = text.begin();
			text_end = text.end();
			// Регуляркой забираем только слова на русском языке и убираем все знаки препинания
			while (u32regex_search(text_start, text_end, result, u32rx)) {
				clear_text << result[1] << " ";
				text_start = result[1].second;
			}
			mystem_input << clear_text.str() << endl;
			clear_text.str("");
		}
		mystem_input.close();
		// БД СМАД'а нам больше не понадобится отключаемся
		smad_db->delete_result();
		smad_db->close();
		delete smad_db;

		// Прогоняем тексты из input через mystem, результаты пишем output
		cout << "Processes by using 'mystem' " << i << " texts\t\t\t\t";
        string mystem_command = "mystem -icwld " + config->mystem_input + " " + config->mystem_output;
        std::system(mystem_command.c_str());
		cout << "[OK]" << endl;
	}
	// Возвращаем имя и id категории
	return category_param;
}

category trend_process::get_texts_from_file (string file_name)
{
	csv_parser* csv_file = new csv_parser();
	csv_file->open(file_name, ";");
	csv_file->print_data();
	delete csv_file;

	category category_param;
	category_param.name = file_name;
	category_param.id = -1;

	return category_param;
}

int trend_process::fill_db_with_words (category category_param)
{
	size_t i = 0;
	string query_string = "";
	string text = "";
	stringstream clear_text;
	stringstream word;
	stringstream part_of_speach;
	string::const_iterator text_start;
	string::const_iterator text_end;

	vector<string> query_strings;
	unordered_map <string, int> words;

	struct word_params {
		string part_of_speach;
		int frequency;
	};
	unordered_map <string, word_params> words_d;

	word_params params;

	char_separator<char> sep(" ");
	tokenizer<char_separator<char>>* words_from_text(string, char_separator<char>);
	tokenizer<char_separator<char>>::iterator next_word;

	ifstream mystem_output;
	u32regex u32rx = make_u32regex("([а-яА-ЯёЁ]+)[=?]([A-Z?]+)");
    smatch result;
	pgsql_connect* dict_db = new pgsql_connect (config, TO_COUT);

	if (dict_db->connect() == true) {
		dict_db->clear_table("category_dictionary");
        dict_db->set_to_zero("categoty_dictionary_word_id_seq");
		mystem_output.open(config->mystem_output);
		if (mystem_output.is_open()) {
			while (getline(mystem_output, text)) {
				// Обрабатываем очередной текст
				text_start = text.begin();
				text_end = text.end();
				// Регуляркой убираем скобки - {}
				while (u32regex_search(text_start, text_end, result, u32rx)) {
					clear_text << result[1] << " ";
					word << result[1];
					part_of_speach << result[2];
					text_start = result[1].second;
					words_d[word.str()].frequency++;
					if (part_of_speach.str() != "?") {
						words_d[word.str()].part_of_speach = part_of_speach.str();
					}
					else { 
						words_d[word.str()].part_of_speach = "SU";
					}
					word.str("");
					part_of_speach.str("");
				}

				// Разбиваем очередной текст на слова и заносим их в umap подсчитывая их кол-во
				text = clear_text.str();
				tokenizer<char_separator<char>> new_words(text, sep);
				for (next_word = new_words.begin(); next_word != new_words.end(); ++next_word) {
					words[*next_word]++; 
				}
				clear_text.str("");
			}
			mystem_output.close();

			// Отправляем в базу слова
			query_string = "BEGIN;\n";	
			i = 0;
			cout << "Started adding " << words.size() << " words in the dictionary\t\t\t";
			/*for (auto &this_word: words) {
				i++;
				query_string += "INSERT INTO category_dictionary (word, frequency, category_name, category_id) VALUES ('" +
								this_word.first + "', " +
								to_string(this_word.second) + ", '" +
								category_param.name + "', " +
								to_string(category_param.id) + ");\n";
				if (i == 50000) {
					query_string += "COMMIT;";
					query_strings.push_back(query_string);
					query_string = "BEGIN;\n";
					i = 0;
				}
			}*/
			for (auto &this_word: words_d) {
				i++;
				query_string += "INSERT INTO category_dictionary (word, frequency, category_name, category_id, part_of_speech) VALUES ('" +
								this_word.first + "', " +
								to_string(this_word.second.frequency) + ", '" +
								category_param.name + "', " +
								to_string(category_param.id) + ", '" + 
								this_word.second.part_of_speach + "');\n";
				if (i == 50000) {
					query_string += "COMMIT;";
					query_strings.push_back(query_string);
					query_string = "BEGIN;\n";
					i = 0;
				}
			}
			cout << "[OK]" << endl;
			query_string += "COMMIT;";
			query_strings.push_back(query_string);
			query_string = "";

			for (auto &this_query_string: query_strings) {
				dict_db->query(this_query_string);
			}
			query_strings.clear();
		}
		else {
			cout << "Could not open file: " << config->mystem_output << endl;
		}
		dict_db->close();
	}
	
	return 1;
}

int trend_process::remove_parts_of_speech (list<string> parts_of_speech)
{
	string query_string = "DELETE FROM category_dictionary as cd WHERE cd.part_of_speech IN (";

	pgsql_connect* dict_db = new pgsql_connect (config, TO_COUT);

	if (dict_db->connect() == true) {
		for (auto &part_of_speech: parts_of_speech) {
			query_string += "'" + part_of_speech + "',";
		}
		// Удалим лишнюю запятую в конце строки
		query_string = query_string.erase(query_string.length() - 1);
		query_string += ");";
	}
	// Удаляем выбранные части речи
	dict_db->query(query_string);
	dict_db->close();
	delete dict_db;

	return 1;
}

int trend_process::remove_word_clusters (list<string> clusters)
{
	string query_string = "DELETE FROM category_dictionary as cd WHERE cd.cluster_id IN (";

	pgsql_connect* dict_db = new pgsql_connect (config, TO_COUT);

	if (dict_db->connect() == true) {
		for (auto &cluster: clusters) {
			query_string += cluster + ",";
		}
		// Удалим лишнюю запятую в конце строки
		query_string = query_string.erase(query_string.length() - 1);
		query_string += ");";
	}
	// Удаляем выбранные кластеры
	dict_db->query(query_string);
	dict_db->close();
	delete dict_db;

	return 1;
}

int trend_process::remove_stop_words (list<string> stop_words)
{
	string query_string = "DELETE FROM category_dictionary as cd WHERE cd.word IN (";

	pgsql_connect* dict_db = new pgsql_connect (config, TO_COUT);

	if (dict_db->connect() == true) {
		for (auto &stop_word: stop_words) {
			query_string += "'" + stop_word + "',";
		}
		// Удалим лишнюю запятую в конце строки
		query_string = query_string.erase(query_string.length() - 1);
		query_string += ");";
	}
	// Удаляем выбранные кластеры
	dict_db->query(query_string);
	dict_db->close();
	delete dict_db;

	return 1;
}

void trend_process::get_available_categories () {
	string query_string = "select sc.name from subcategorys as sc;";

	mysql_connect* smad_db = new mysql_connect (config);

	if (smad_db->connect() == true) {
		smad_db->query(query_string);
		std::cout << "Available categories:" << std::endl;
		while (smad_db->get_result_row() == true) {
			std::cout << smad_db->get_result_value(0) << std::endl;
		}
		std::cout << std::endl;

		query_string = "show tables from smad;";
		smad_db->query(query_string);
		while (smad_db->get_result_row() == true) {
            std::cout << smad_db->get_result_value(0) << std::endl;
        }
		std::cout << std::endl;

		query_string = "show columns from emot_heap;";
        smad_db->query(query_string);
        while (smad_db->get_result_row() == true) {
            std::cout << smad_db->get_result_value(0) << std::endl;
        }
        std::cout << std::endl;

		smad_db->close();
		delete smad_db;
	}
}

int trend_process::set_stop_word_list (string stop_word_file_name)
{
	ofstream stop_word_file;
	string query_string = "SELECT cd.word, cd.frequency FROM category_dictionary as cd ORDER BY cd.frequency DESC LIMIT 1000;";

	stop_word_file.open(stop_word_file_name);

	if (stop_word_file.is_open()) {
		pgsql_connect* dict_db = new pgsql_connect (config, TO_COUT);
		if (dict_db->connect() == true) {
			dict_db->query(query_string);
			for (size_t i = 0; i < dict_db->rows_count(); i++) {
				stop_word_file << dict_db->get_value(i, 0) << std::endl;
			}
			dict_db->close();
		}
		else {
			std::cout << "Can not establish a database connection" << std::endl;
		}
		stop_word_file.close();
	}
	else {
		std::cout << "Can not open file: " << stop_word_file_name << std::endl;
	}

	return 1;
}

int trend_process::uploading_data (string output_file_name)
{
	ofstream output_file;
	string query_string = "SELECT * FROM category_dictionary as cd;";
	output_file.open(output_file_name);

	if (output_file.is_open()) {
		pgsql_connect* dict_db = new pgsql_connect (config, TO_COUT);
		if (dict_db->connect() == true) {
			dict_db->query(query_string);
			for (size_t i = 0; i < dict_db->rows_count(); i++) {
				for (size_t j = 0; j < dict_db->columns_count(); j++) {
					//int opop = dict_db->get_result(i, j, int());
					output_file << dict_db->get_value(i, j) << ";";	
				}
				output_file << std::endl;
			}
			dict_db->close();
		}
		else {
			std::cout << "Can not establish a database connection" << std::endl;
		}
		output_file.close();
	}
	else {
		std::cout << "Can not open file: " << output_file_name << std::endl;
	}

	return 1;
}


void trend_process::do_kohonen_network ()
{
	::boost::int32_t 	R = 3,
						C = 3;

	::boost::int32_t no_epochs = 1000;

	// Определяем контейнер для хранения текстов
	typedef std::vector <double> text_vector;
	typedef std::vector <text_vector> text_vectors;
	// Здесь все вектора текстов:
	text_vector vector;
	text_vectors vectors;
	text_vectors c_vectors;

	// Зполним данные
	csv_parser* csv_file = new csv_parser();
	csv_file->open("testfile", ";");
	cout << "====================" << endl;
	/*for (auto &data: csv_file->csv_data) {
		for (auto &parameter: data.second) {
			vector.push_back(stod(parameter.second));
		}
		vectors.push_back(vector);
		vector.clear();
	}*/
	//c_vectors = csv_file->simple_csv_data;
	double d = 0.0;	// коэффициент нормализации
	for (auto &data: csv_file->simple_csv_data) {
		// коэффициент нормализации
		for (auto &parameter: data) {
			d += pow(parameter, 2);
		}
		d = sqrt(d);
		for (auto &parameter: data) {
			vector.push_back(parameter / d);
		}
		d = 0.0;
		vectors.push_back(vector);
		vector.clear();
	}

	c_vectors = vectors;

	delete csv_file;

	/*for (auto &d: vectors) {
		for (auto &u: d) {
			cout << u << " ";
		}
		cout << endl << "----------" << endl;
	}*/

	//std::srand (static_cast<unsigned int> (time(nullptr)));

	// Функция активации Коши
	typedef ::neural_net::Cauchy_function <text_vector::value_type, text_vector::value_type, ::boost::int32_t> cauchy_activation_function;

	// Функция активации Гаусса
	typedef ::neural_net::Gauss_function <text_vector::value_type, text_vector::value_type, ::boost::int32_t> gauss_activation_function;

	// Объявляем функцию активации (Гаусс/Коши):
	//cauchy_activation_function activation_function (20.0, 1);
	gauss_activation_function activation_function(20.0, 1);

	// Евклидово расстояние
	typedef ::distance::Euclidean_distance_function <text_vector> euclidean_distance_function;
	euclidean_distance_function distance_function;

	/*/ and weighted Euclidean distance function
	typedef distance::Weighted_euclidean_distance_function <text_vector, text_vector> weighted_euclidean_distance_function;

	text_vector coefs;

	neural_net::Ranges <text_vectors> data_ranges (*vectors.begin());
	data_ranges(vectors);

	//preparing proper parameters for weighted distance
	text_vector::const_iterator pos_max = data_ranges.get_max().begin();
	text_vector::const_iterator pos_min = data_ranges.get_min().begin();

	for (; pos_max != data_ranges.get_max().end(); ++pos_max, ++pos_min) {
		coefs.push_back (
			::operators::inverse (
				( *pos_max - *pos_min ) * ( *pos_max - *pos_min ) 
			)
		); // weight for i-th axis
	}

	weighted_euclidean_distance_function weighted_distance_function (&coefs);
	*/
	// Нейрон
	typedef ::neural_net::Basic_neuron <
		gauss_activation_function,
		//cauchy_activation_function,
		euclidean_distance_function
	> kohonen_neuron;
	
	kohonen_neuron neuron (*(vectors.begin()), activation_function, distance_function);
	
	//typedef neural_net::Basic_neuron <gauss_activation_function, weighted_euclidean_distance_function> kohonen_neuron;
	//kohonen_neuron neuron(*(vectors.begin()), activation_function, weighted_distance_function);

	// Строим сеть
	typedef ::neural_net::Rectangular_container<kohonen_neuron> kohonen_network;
	kohonen_network neural_network;

	neural_net::Internal_randomize IR;
	//neural_net::External_randomize ER;

	neural_net::generate_kohonen_network (R, C, activation_function, distance_function, vectors, neural_network, IR);
	//neural_net::generate_kohonen_network (R, C, activation_function, weighted_distance_function, vectors, neural_network, ER);

	/*/ Обучение сети (WTA) -----------------------------------------------------------------------------------------#
	typedef neural_net::Wta_proportional_training_functional < text_vector, double, int > wta_training_functional;
	wta_training_functional training_functional(0.2, 0);

	typedef neural_net::Wta_training_algorithm 
	<
		kohonen_network,
		text_vector,
		text_vectors::iterator,
		wta_training_functional
	> wta_training_algorithm;

	wta_training_algorithm training_algorithm(training_functional);

	for (size_t i = 0; i < no_epochs; ++i )
	{
		training_algorithm (vectors.begin(), vectors.end(), &neural_network);
		//std::random_shuffle (vectors.begin(), vectors.end());
	}*/

	// Обучение сети (WTM) -----------------------------------------------------------------------------------------#
	// Топология
	typedef ::neural_net::Hexagonal_topology <boost::int32_t> hexagonal_topology;
	typedef ::neural_net::City_topology <boost::int32_t> city_topology;
	typedef ::neural_net::Max_topology <boost::int32_t> max_topology;
	//hexagonal_topology network_topology (neural_network.get_no_rows());
	//city_topology network_topology;
	max_topology network_topology;

	// Gaussian hat function:
	typedef ::neural_net::Gauss_function <text_vector::value_type, text_vector::value_type, ::boost::int32_t> gauss_function_space;
	gauss_function_space function_space (100, 1);

	//typedef ::neural_net::Cauchy_function <text_vector::value_type, text_vector::value_type, ::boost::int32_t > cauchy_function_space;
	//cauchy_function_space function_space (100, 1);

	typedef ::neural_net::Gauss_function <boost::int32_t, text_vector::value_type, boost::int32_t> gauss_function_network;
	gauss_function_network function_network (10, 1);

	//typedef ::neural_net::Cauchy_function < ::boost::int32_t, text_vector::value_type, ::boost::int32_t > cauchy_function_network;
	//cauchy_function_network function_network (10, 1);

	// Веса
	typedef ::neural_net::Classic_training_weight
	<
		text_vector,
		::boost::int32_t,
		gauss_function_network,
		gauss_function_space,
		//cauchy_function_network,
		//cauchy_function_space,
		//hexagonal_topology,
		max_topology,
		//city_topology,
		euclidean_distance_function,
		::boost::int32_t
	> classic_training_weight;

	//classic_training_weight training_weight (function_network, function_space, network_topology, distance_function);

	typedef neural_net::Experimental_training_weight
	<
		text_vector,
		::boost::int32_t,
		gauss_function_network,
		gauss_function_space,
		//cauchy_function_network,
		//cauchy_function_space,
		//hexagonal_topology,
		max_topology,
		//city_topology,
		euclidean_distance_function,
		::boost::int32_t,
		double
	> experimental_training_weight;

	experimental_training_weight training_weight (function_network, function_space, network_topology, distance_function, 1, 2);

	// Функция обучения
	typedef ::neural_net::Wtm_classical_training_functional
	<
		text_vector,
		double,
		::boost::int32_t,
		::boost::int32_t,
		experimental_training_weight
		//classic_training_weight
	> wtm_classical_training_functional;

	wtm_classical_training_functional training_functional (training_weight, 0.3);

	// Алгоритм обучения

	typedef ::neural_net::Wtm_training_algorithm
	<
		kohonen_network,
		text_vector,
		text_vectors::iterator,
		wtm_classical_training_functional,
  		::boost::int32_t
	> wtm_training_algorithm;

	wtm_training_algorithm training_algorithm (training_functional);

	/*for (::boost::int32_t i = 0; i < no_epochs; ++i ) {
		// train network using vectors
		training_algorithm (vectors.begin(), vectors.end(), &neural_network);

		// decrease sigma parameter in network will make training proces more sharpen with each epoch,
		// but it have to be done slowly :-)
		training_algorithm.training_functional.generalized_training_weight.network_function.sigma *= 2.0/3.0;

		// shuffle vectors
		::std::random_shuffle(vectors.begin(), vectors.end());
	}*/

	::boost::int32_t const border = 5;
	for (size_t i = 0; i < no_epochs; ++i)
	{
		if (i < border) {
			// use linear function to modify parameters
			training_algorithm.training_functional.generalized_training_weight.parameter_1
				= - 1.0 / static_cast <double> ( border - 1 ) * static_cast <double> ( i ) + 2.0;
			training_algorithm.training_functional.generalized_training_weight.parameter_0
				= training_algorithm.training_functional.generalized_training_weight.parameter_1 - 1.0;
		}
		else {
			// set "normal" parameters for training functors
			training_algorithm.training_functional.generalized_training_weight.parameter_1 = 1;
			training_algorithm.training_functional.generalized_training_weight.parameter_0 = 0;
		}
		
		training_algorithm(vectors.begin(), vectors.end(), &neural_network);
		training_algorithm.training_functional.generalized_training_weight.network_function.sigma *= 2.0/4.0;
		//std::random_shuffle (vectors.begin(), vectors.end());
	}

	// Использование сети ------------------------------------------------------------------------------------------#
	std::cout << "Printing network:" << std::endl;
	neural_net::print_network (std::cout, neural_network, *(vectors.begin()));

	double current_weight = 0.0;
	double minimun_weight = 10.0;
	double maximum_weight = 0.0;
	//unordered_map<size_t, size_t> cluster_id;
	std::unordered_map <std::string, int> counter;
	struct cluster_coord
	{
		size_t x;
		size_t y;
	} cluster;

	std::ofstream result;
	result.open("clusters_result.csv");
	for (auto vec: c_vectors) {
		for (boost::int32_t i = 0; i < R; i++) {
			for (boost::int32_t j = 0; j < C; j++) {
				current_weight = neural_network(i,j)(vec);
				if (current_weight > maximum_weight) {
					maximum_weight = current_weight;
					cluster.x = i;
					cluster.y = j;
				}
			}
		}
		std::cout << "[" << cluster.x << ":" << cluster.y << " # " << maximum_weight << "] --> ";
		result << cluster.x << ":" << cluster.y << endl;
		for (auto &unit: vec) {
			std::cout << unit << " ";
		}
		std::cout << std::endl << std::endl;
		counter[to_string(cluster.x) + ":" + to_string(cluster.y)]++;
		maximum_weight = 0.0;
	}
	for (auto c: counter) {
		std::cout << c.first << " # " << c.second << std::endl;
	}
	//neural_net::print_network_weights (std::cout, neural_network);

}