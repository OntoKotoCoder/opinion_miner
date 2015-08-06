#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <list>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>

typedef std::unordered_map<int, std::unordered_map<int, std::string>> data;
typedef std::vector<std::vector<double>> simple_data;

class csv_parser
{
private:

public:
	data csv_data;
	simple_data simple_csv_data;
	size_t row_count;
	std::unordered_map <int, int> cell_count;

	csv_parser ();
	~csv_parser ();

	int open (std::string csv_file_name, const char* delimiter);
	int print_data ();

private:
	std::ifstream csv_file;
};