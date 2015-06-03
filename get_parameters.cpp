#include "get_parameters.h"

get_parameters::get_parameters (char* config_file_name)
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
}

void get_parameters::get_directories_params ()
{
	// [mystem:]
	mystem_input = get_param("directories.mystem.input_file");
	mystem_output = get_param("directories.mystem.output_file");

	// [log:]
	query_log = get_param("directories.log.query_log");
	worker_log = get_param("directories.log.worker_log");
	error_log = get_param("directories.log.error_log");
	
	// [data:]
	last_date = get_param("directories.data.last_date."); 
	vector_space_file_name = get_param("directories.data.vector_space_file");
	model_file_name = get_param("directories.data.model_file");
}

void get_parameters::get_servers_params () 
{
	// [smad_db:]
	smad_db_host = get_param("servers.smad_db.host");
	smad_db_name = get_param("servers.smad_db.name");
	smad_db_user = get_param("servers.smad_db.user");
	smad_db_pass = get_param("servers.smad_db.password");
	// [dictionary_db:]
	dict_db_host = get_param("servers.dictionary_db.host");
	dict_db_name = get_param("servers.dictionary_db.name");
	dict_db_user = get_param("servers.dictionary_db.user");
	dict_db_pass = get_param("servers.dictionary_db.password");
	dict_db_encod = get_param("servers.dictionary_db.encoding");
	// [gearman:]
	gearman_host = get_param("servers.gearman.host");
	gearman_port = get_int_param("servers.gearman.port");
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
