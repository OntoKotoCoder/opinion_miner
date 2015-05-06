#include "pgsql_connect.h"
#include "mysql_connect.h"
#include "get_parameters.h"
#include "svm/svm.h"

class process
{
private:

public:
	get_parameters* config;

	process ();
	void fill_db_with_training_set();
	void fill_db_with_n_gramms();
	void calculate_idf();
	void calculate_vector_space();
	void start_svm_train();
	void start_svm_predict();

private:
	struct svm_parameter param;
	struct svm_problem prob;
	struct svm_model *model;
	struct svm_node *v_space;
	int max_nr_attr = 256;

	void predict(char* v_space_file_name, char* result_file_name);
	void read_v_space_file(string v_space_file_name);
	void get_svm_parameters();
	void exit_input_error(int line_num);
	void print_progress(char completed_symbol, char not_completed_symbol, unsigned int completed_count, unsigned int progress_length);
	void print_line (size_t symbols_count, char symbol);
	void print_line (char symbol);
};
