#include <string>
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
	// general
	int texts_limit;
	unsigned int n_gramm_size;
	int number_of_classes;
	bool use_tf;
	
	string v_space_file_name;
	string model_file_name;

	// smad db
	string 	smad_db_host,
		smad_db_name,
		smad_db_user,
		smad_db_pass;
	
	string 	dict_db_host,
		dict_db_name,
		dict_db_user,
		dict_db_pass,
		dict_db_encod;

	int svm_type,
	    kernel_type;
	enum svms {C_SVC, NU_SVC};
	enum kernels {LINEAR, POLY, RBF, SIGMOID};
	unordered_map<string, enum svms> svm_types;
	unordered_map<string, enum kernels> kernel_types;
	
	get_parameters (const char* config_file_name);
	void get_general_params ();
	void get_smad_db_params ();
	void get_dict_db_params ();
	void get_svm_params ();

private:
	string get_param (string param_name);
	int get_int_param (string param_name);
	bool get_bool_param (string param_name);
};
