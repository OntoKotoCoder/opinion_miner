#include <iostream>
#include <cstdlib>

#include <getopt.h>

#include <boost/version.hpp>

#include "main.h"
#include "process.h"

using namespace std;

get_parameters* config = new get_parameters("/opt/opinion_miner/general.cfg");

int main (int argc, char **argv) 
{
	bool do_fill_db_with_training_set = false,
	     do_update_db_with_training_set = false,
	     do_fill_db_with_training_set_from_file = false,
	     do_fill_db_with_n_gramms = false,
	     do_calculate_vector_space = false,
	     do_start_svm_train = false,
	     do_start_svm_predict = false,
	     do_calc_emotion = false;

	config->get_general_params();
	config->get_servers_params();
	config->get_directories_params();
	config->get_svm_params();

	int option;
	int option_index;
	const char* short_options = "htufnvspl:d:m:e";
	const struct option long_options[] = {
		{"help", no_argument, nullptr, 'h'},
		{"fill_texts", no_argument, nullptr, 't'},
		{"update_texs", no_argument, nullptr, 'u'},
		{"texts_from_file", no_argument, nullptr, 'f'},
		{"fill_n_gramms", no_argument, nullptr, 'n'},
		{"calc_vector", no_argument, nullptr, 'v'},
		{"svm_train", no_argument, nullptr, 's'},
		{"svm_predict", no_argument, nullptr, 'p'},
		{"limit", required_argument, nullptr,'l'},
		{"data_file", required_argument, nullptr,'d'},
		{"model_file", required_argument, nullptr,'m'},
		{"calc_emotion", no_argument, nullptr, 'e'},
		{nullptr, 0, nullptr, 0}
	};
	ofstream log;
	log.open("/var/log/opinion_miner/worker.log");
	log << get_time() << " WORKR # Start Opinion Miner | version 0.1.1" << endl;	
	//cout << "Sentiment Analysis" << endl;
	//cout << "Version: 0.1" << endl;
	//get_boost_version();
	
	process* proc = new process();
	if (argc == 1) {
		get_help();
	}
	while ((option = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch(option) {
			case 'l': {
				config->texts_limit = atoi(optarg);
				break;		
			}
			case 'd': {
				config->vector_space_file_name = optarg;
				break; 
			}
			case 'm': {
				config->model_file_name = optarg;
				break; 
			}
			case 'h': {
				get_help(); cout << endl;
				break;	
			}
			case 't': {
				do_fill_db_with_training_set = true;
				break;
			}
			case 'u': {
				do_update_db_with_training_set = true;
				break;
			}
			case 'f': {
				do_fill_db_with_training_set_from_file = true;
				break;
			}
			case 'n': {
				do_fill_db_with_n_gramms = true;
				break;
			}
			case 'v': {
				do_calculate_vector_space = true;
				break;
			}
			case 's': {
				do_start_svm_train = true;
				break;
			}
			case 'p': {
				do_start_svm_predict = true;
				break;
			}
			case 'e': {
				do_calc_emotion = true;
				break;
			}
		}
	}
	if (do_fill_db_with_training_set == true) {
		cout << ">> Populate the database with texts of starting sample" << endl;
		print_line('='); 
		proc->fill_db_with_training_set();
		print_line('=');
		cout << "Finished!" << endl << endl;
	}
	if (do_update_db_with_training_set == true) {
		cout << ">> Update the database with texts of starting sample" << endl;
		print_line('='); 
		proc->update_db_with_training_set();
		print_line('=');
		cout << "Finished!" << endl << endl;
	}
	if (do_fill_db_with_training_set_from_file == true) {
		cout << ">> Populate the database with texts of starting sample" << endl;
		print_line('='); 
		proc->fill_db_with_training_set_from_file();
		print_line('=');
		cout << "Finished!" << endl << endl;
	}
	if (do_fill_db_with_n_gramms == true) {	
		cout << ">> Populate the database with n_gramms" << endl;
		print_line('=');
		proc->fill_db_with_n_gramms();
		print_line('=');
		cout << "Finished!" << endl << endl;
	}
	if (do_calculate_vector_space == true) {
		cout << ">> Calculate vector space" << endl;
		print_line('=');
		proc->calculate_vector_space();
		print_line('=');
		cout << "Finished!" << endl << endl;
	}
	if (do_start_svm_train == true) {
		cout << ">> Start SVM train" << endl;
		print_line('=');
		proc->start_svm_train();
		print_line('=');
		cout << "Finished!" << endl << endl;
	}
	if (do_start_svm_predict == true) {
		cout << ">> Start SVM predict" << endl;
		print_line('=');
		proc->start_svm_predict();
		print_line('=');
		cout << "Finished!" << endl << endl;
	}
	if (do_calc_emotion == true) {
		proc->start_calc_emotion();
	}
}

void print_line (size_t symbols_count, char symbol) 
{
	for (size_t i = 0; i < symbols_count; i ++) {
		cout << symbol;
	}
	cout << endl;
}
void print_line (char symbol)
{
	size_t symbols_count = atoi(getenv("COLUMNS"));
	for (size_t i = 0; i < symbols_count; i ++) {
                cout << symbol;
        }
        cout << endl;
}
void get_boost_version ()
{
	std::cout << "Boost version: "     
		  << BOOST_VERSION / 100000     << "."  // major version
		  << BOOST_VERSION / 100 % 1000 << "."  // minior version
		  << BOOST_VERSION % 100                // patch level
		  << std::endl;
}
char* get_time ()
{
        time_t rawtime;
        struct tm* timeinfo;
        char* _buffer = new char[17];

        time (&rawtime);
        timeinfo = localtime (&rawtime);

        strftime (_buffer,80,"%d.%m.%y-%H:%M:%S",timeinfo);
        return _buffer;
}
void get_help ()
{
	cout << "О программе:" << endl;
	print_line('=');
	cout << " -t : Инициализирует заполнение базы данных текстами начальной выборки." << endl
	     << " -f : Инициализирует заполнение БД текстами начальной выборки из файла /opt/opinion_miner/data/texts. Специальная опция для добавления в БД старой коллекции текстов." << endl 
	     << " -n : Инициализирует заполнение базы данных N-граммами. Производится расчёт IDF." << endl
	     << " -s : Инициализирует обучение SVM классификатора. На выходе формируется файл vector_space.model. Имя файла задаётся через конфигурационный файл." << endl
	     << " -p : Инициализирует оценку тональность текстов. В качестве модели используется файл заданный в конфигурационном файле (по умолчанию vector_space.model)" << endl
	     << " -l : Определяет кол-во текстов забираемых из базы данных СМАД." << endl
	     << " -d : Определяет файл векторного пространства для обучения SVM классификатора, или для определения тональности." << endl
	     << " -m : Определяет файл модели, используемой для определения тональности текстов. Файл модели создаётся при обучении SVM классификатора." << endl
	     << " -h : Выводит информацию о программе." << endl
	     << "-----------------------" << endl
	     << " Конфигурационный файл: /opt/sentiment_analysis/general.cfg" << endl;
	print_line('=');
}
