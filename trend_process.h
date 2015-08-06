#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cmath>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/cstdint.hpp>

#include "knnl/std_headers.hpp"
#include "knnl/neural_net_headers.hpp"

#include "pgsql_connect.h"
#include "mysql_connect.h"
#include "get_parameters.h"
#include "k_means.h"
#include "csv_parser.h"

using namespace std;
using namespace boost;

typedef unordered_map<string, int> new_category;

struct category {
	string name;
	int id;
};

enum data_input {FROM_DB, FROM_FILE};

class trend_process
{
private:

public:
	trend_process (get_parameters* config);
	~trend_process ();

	void start_fill_db_with_words (string name, bool with_text, bool with_title, int data_input);
	void start_clustering (int clusters_count);

	//Очистка словаря:
	int remove_word_clusters (list<string> clusters);
	int remove_parts_of_speech (list<string> parts_of_speech);
	int remove_stop_words (list<string> stop_words);

	//Вспомогательные функции
	void get_available_categories ();
	int set_stop_word_list (string stop_word_file_name);
	int uploading_data (string output_file_name);
	
	// Сеть Кохонена
	void do_kohonen_network ();

private:
	// Параметры:
	get_parameters* config;
	bool with_title = false;
	bool with_text = false;

	int fill_db_with_words (category category_param);
	category get_texts_from_category (string category_name);
	category get_texts_from_file (string file_name);
};
