#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "pgsql_connect.h"
#include "mysql_connect.h"
#include "get_parameters.h"
#include "svm/svm.h"

using namespace std;
using namespace boost;

typedef unordered_map<int, string> new_guids;
typedef unordered_map<string, int> new_emotions;

class processd
{
private:
	get_parameters* config;

public:
	processd (get_parameters* config);
	~processd ();
	int calculate_tonality ();

private:
	struct svm_model *model;
	struct svm_node *v_space;
	//int max_nr_attr = 256;
	ofstream error_log;
	ofstream worker_log;

	// Основные функции
	new_guids get_texts ();
	int create_vector_space (new_guids guids);
	new_emotions define_tonality (string vector_space_file_name);
	int send_tonality(new_emotions emotions);

	// Вспомогательные функции
	char* get_time ();
};