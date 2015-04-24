#include <string>
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
	
	get_parameters (const char* config_file_name);
	void get_general_params ();
	void get_smad_db_params ();
	void get_dict_db_params ();

private:
	string get_param (string param_name);
	int get_int_param (string param_name);
};
