#include <cstdlib>
#include <iostream>
#include <locale>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/locale/encoding_utf.hpp>

#include "pgsql_connect.h"
#include "mysql_connect.h"
#include "get_parameters.h"
#include "svm/svm.h"

using namespace std;
using namespace boost;
using boost::locale::conv::utf_to_utf;

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
	int calculate_tonality_from_gearman (string texts);

private:
	struct svm_model *model;
	struct svm_node *v_space;
	ofstream error_log;
	ofstream worker_log;

	// Основные функции
	new_guids get_texts ();
	new_guids get_last_texts ();
	new_guids get_texts_from_gearman (string texts);
	
	int create_vector_space (new_guids guids);
	new_emotions define_tonality (string vector_space_file_name);
	int send_tonality(new_emotions emotions);

	// Вспомогательные функции
	char* get_time ();
	std::wstring utf8_to_wstring(const std::string& str);
};
