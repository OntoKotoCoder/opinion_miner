#include "pgsql_connect.h"
#include "mysql_connect.h"
#include "get_parameters.h"

class process
{
private:

public:

	process ();
	void fill_db_with_training_set();
	void fill_db_with_n_gramms();
	void calculate_idf();
	void calculate_vector_space();

private:
	void print_progress(char completed_symbol, char not_completed_symbol, int completed_count, int progress_length);
};
