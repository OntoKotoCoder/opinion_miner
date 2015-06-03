#ifndef GET_PARAMETERS_H
#define GET_PARAMETERS_H

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>

#include <libconfig.h++>

using namespace std;
using namespace libconfig;

class get_parameters
{
private:
	Config config;
	const char* config_file;
public:
	// [general:]
	int 			texts_limit;
	unsigned int	n_gramm_size;
	int 			number_of_classes;
	bool 			use_tf;
	
	// [directories:]
	// [mystem:]
	string			mystem_input,
					mystem_output;
	// [log:]
	string			query_log,
					worker_log,
					error_log;
	// [data:]
	string			last_date,
					vector_space_file_name,
					model_file_name;

	// [servers:]
	// [smad_db:]
	string			smad_db_host,
					smad_db_name,
					smad_db_user,
					smad_db_pass;
	// [dictionary_db:]
	string			dict_db_host,
					dict_db_name,
					dict_db_user,
					dict_db_pass,
					dict_db_encod;
	// [gearman:]
	string			gearman_host,
					gearman_port;
	
	// [svm:]
	int				svm_type,
	    			kernel_type;
	unsigned int 	C;
	double 			nu;
	
	enum svms {C_SVC, NU_SVC};
	enum kernels {LINEAR, POLY, RBF, SIGMOID};
	unordered_map<string, enum svms> svm_types;
	unordered_map<string, enum kernels> kernel_types;
	
	// Конструктор класса
	get_parameters (char* config_file_name);
	
	// Функции чтения различных блоков конфигурационного файла:
	// ========================================================
	// [general:] Основные параметры
	void get_general_params ();
	// [directories:] Пути к различным файлами, которые использует программа
	void get_directories_params ();
	// [servers:] Параметры подключения к базам данных и серверу gearman
	void get_servers_params ();
	// [svm:] Параметры SVM классификатора
	void get_svm_params ();

private:
	string get_param (string param_name);
	int get_int_param (string param_name);
	double get_double_param (string param_name);
	bool get_bool_param (string param_name);
};
#endif
