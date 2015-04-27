#include <iostream>
#include <cstdlib>

#include <getopt.h>

#include <boost/version.hpp>

#include "main.h"
#include "process.h"

using namespace std;
using namespace std;

int main (int argc, char **argv) 
{
	int option;
	int option_index;
	const char* short_options = "htl:";
	const struct option long_options[] = {
		{"help", no_argument, nullptr, 'h'},
		{"limit", required_argument, nullptr,'l'},
		{"fill_texts", no_argument, nullptr, 't'},
		{nullptr, 0, nullptr, 0}
	};
	
	cout << "Sentiment Analysis" << endl;
	cout << "version: 0.1" << endl;
        
	//get_boost_version();
	
	process* proc = new process();
	
	while ((option = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch(option) {
			case 'h': {
				get_help();
				cout << endl;
				break;	
			}
			case 'l': {
				//config->texts_limit = atoi(optarg);
				break;		
			}
			case 't': {
				cout << ">> Populate the database with texts of starting sample" << endl;
				print_line('='); 
				proc->get_texts_with_emotion();
				print_line('=');
				cout << "Finished!" << endl << endl;
				break;
			}
		}
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
void get_help ()
{
	system("export COLUMNS");
	cout << "О программе:" << endl;
	print_line('=');
	cout << " -t : опция инициализирует заполнение базы данных текстами начальной выборки" << endl
	     << " -l : определяет кол-во текстов забираемых из базы данных СМАД" << endl
	     << " -h : выводит информацию о программе" << endl
	     << " конфигурационный файл: /opt/sentiment_analysis/general.cfg" << endl;
	print_line('=');
}
