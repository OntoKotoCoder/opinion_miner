#include "csv_parser.h"

csv_parser::csv_parser ()
{
	row_count = 0;
}

csv_parser::~csv_parser ()
{
	csv_data.clear();
}

int csv_parser::open (std::string csv_file_name, const char* delimiter)
{
	boost::u32regex u32rx = boost::make_u32regex("(;;)");
	std::string format_line("; ;");
	std::vector<double> line_buffer;

	std::string line = "";

	csv_file.open(csv_file_name);
	
	if (csv_file.is_open()) {
		std::cout << "The file is successfully opened: " << csv_file_name << std::endl;
		while (getline(csv_file, line)) {
			if (line != "") {
				char *cell, *endptr;
				row_count++; cell_count[row_count] = 1;
				//std::cout << row_count << std::endl;
				//std::cout << cell_count[row_count] << std::endl;

				// Без двойного прохода получается: ; ;; ; ;; ; ;;
				line = boost::u32regex_replace(line, u32rx, format_line);
				line = boost::u32regex_replace(line, u32rx, format_line);
				
				//std::cout << line << std::endl;
				cell = strtok_r(&line[0], ";", &endptr);
				//std::cout << cell << ",";
				csv_data[row_count][cell_count[row_count]] = std::string(cell);
				line_buffer.push_back(atof(cell));
				
				while (true) {
					cell = strtok_r(endptr, ";", &endptr);
					//std::cout << cell << ",";
					if (cell == nullptr) {
						//std::cout << "bye,bye" << std::endl;
						//std::cout << std::endl;
						break;
					}
					else {
						cell_count[row_count]++;
						csv_data[row_count][cell_count[row_count]] = std::string(cell);
						line_buffer.push_back(atof(cell));
					}
				}
				simple_csv_data.push_back(line_buffer);
				line_buffer.clear();
			}
		}
	}	
	else {
		std::cout << "Unable to open file: " << csv_file_name << std::endl;
	}

	return 1;
}

int csv_parser::print_data ()
{
	for (size_t i = 1; i <= csv_data.size(); i++) {
		std::cout << "LINE [" << i << "]─┐" << std::endl;
		for (size_t j = 1; j <= csv_data[i].size(); j++) {
			if (j != csv_data[i].size()) {
				std::cout << "\t ├─[" << j << "] " << csv_data[i][j] << std::endl;
			}
			else {
				std::cout << "\t └─[" << j << "] " << csv_data[i][j] << std::endl;
			}
		}
		std::cout << std::endl;
	}	

	return 1;
}