#include <iostream>
#include <string>
#include <sstream>

#include "get_parameters.h"

using namespace std;

get_parameters::get_parameters (const char* config_file_name)
{
	config_file = config_file_name;
	texts_limit = 0;
}

void get_parameters::get_general_params () 
{
	texts_limit = get_int_param("general.texts_limit");
}

void get_parameters::get_smad_db_params () 
{
	smad_db_host = get_param("smad_db.host");
	smad_db_name = get_param("smad_db.name");
	smad_db_user = get_param("smad_db.user");
        smad_db_pass = get_param("smad_db.password");
}

void get_parameters::get_dict_db_params ()
{
	dict_db_host = get_param("dictionary_db.host");
	dict_db_name = get_param("dictionary_db.name");
	dict_db_user = get_param("dictionary_db.user");
	dict_db_pass = get_param("dictionary_db.password");
	dict_db_encod = get_param("dictionary_db.encoding");
}

string get_parameters::get_param (string param_name)
{
	string param_value;

	try {
		config.readFile(config_file);
	}
	catch (const FileIOException &fioex) {
		cerr << "I/O error while reading file." << endl;
	}
	catch (const ParseException &pex) {
		cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << endl;
	}

	try {
		param_value = config.lookup(param_name).c_str();;
	}
	catch(const SettingNotFoundException &nfex) {
		cerr << "No '" << param_name << "' setting in configuration file." << endl;
	}

	return param_value;
}
int get_parameters::get_int_param (string param_name)
{
	int param_value = 0;

	try {
		config.readFile(config_file);
	}
	catch (const FileIOException &fioex) {
		cerr << "I/O error while reading file." << endl;
	}
	catch (const ParseException &pex) {
		cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << endl;
	}

	try {
		param_value = config.lookup(param_name);
	}
	catch(const SettingNotFoundException &nfex) {
		cerr << "No '" << param_name << "' setting in configuration file." << endl;
	}

	return param_value;
}
