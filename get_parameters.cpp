#include "get_parameters.h"

using namespace std;

get_parameters::get_parameters (const char* config_file_name)
{
	config_file = config_file_name;
	texts_limit = 0;
	svm_types = { {"C_SVC", C_SVC}, {"NU_SVC", NU_SVC} };
	kernel_types = { {"LINEAR", LINEAR}, {"POLY", POLY}, {"RBF", RBF}, {"SIGMOID", SIGMOID} }; 
}

void get_parameters::get_general_params () 
{
	texts_limit = get_int_param("general.texts_limit");
	n_gramm_size = get_int_param("general.n-gramm_size");
	number_of_classes = get_int_param("general.number_of_classes");
	use_tf = get_bool_param("general.calculate_with_tf");
	v_space_file_name = get_param("general.vector_space_file");
	model_file_name = get_param("general.model_file");
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
void get_parameters::get_svm_params()
{
	svm_type = svm_types[get_param("svm.svm_type")];
	kernel_type = kernel_types[get_param("svm.kernel_type")];
	nu = get_double_param("svm.nu");
}

string get_parameters::get_param (string param_name)
{
	string param_value;

	try {
		config.readFile(config_file);
	}
	catch (const FileIOException &fioex) {
		cerr << "I/O error while reading configuration file." << endl;
	}
	catch (const ParseException &pex) {
		cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << endl;
	}

	try {
		param_value = config.lookup(param_name).c_str();
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
		cerr << "I/O error while reading configuration file." << endl;
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

double get_parameters::get_double_param (string param_name)
{
	double param_value = 0;

	try {
		config.readFile(config_file);
	}
	catch (const FileIOException &fioex) {
		cerr << "I/O error while reading configuration file." << endl;
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

bool get_parameters::get_bool_param (string param_name)
{
	bool param_value = false;

	try {
		config.readFile(config_file);
	}
	catch (const FileIOException &fioex) {
		cerr << "I/O error while reading configuration file." << endl;
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
