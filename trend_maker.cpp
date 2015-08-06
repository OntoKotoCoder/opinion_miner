#include <iostream>

#include <getopt.h>

#include "opinion_miner.h"
#include "trend_process.h"

using namespace std;

get_parameters* config = new get_parameters("/opt/opinion_miner/general-research.cfg");

int main (int argc, char** argv)
{
	bool with_title = false;
	bool with_text = false;

	bool get_available_categories = false;
	bool set_stop_word_list = false;
	string stop_words_file_name = "";
	bool upload_data = false;
	string upload_data_file_name = "";

	string category = "";
	string file_name = "";
	int clusters = 0;

	bool do_kohonen_clustering = false;
	
	char_separator<char> sep(",");
	tokenizer<char_separator<char>>* words(string, char_separator<char>);
	
	list<string> clusters_to_remove;
	list<string> lexical_classes_to_remove;
	list<string> stop_words;

	int option;
    int option_index;
    const char* short_options = "ahts:c:l:kT:F:C:S:U:";
    const struct option long_options[] = {
    	{"available", no_argument, nullptr, 'a'},
        {"with_title", no_argument, nullptr, 'h'},
        {"with_text", no_argument, nullptr, 't'},
        {"remove_stop_words", required_argument, nullptr, 's'},
        {"remove_clusters", required_argument, nullptr, 'c'},
        {"remove_lexical_classes", required_argument, nullptr, 'l'},
        {"from_category", required_argument, nullptr, 'T'},
        {"from_file", required_argument, nullptr, 'F'},
        {"clusters", required_argument, nullptr, 'C'},
        {"upload_data", required_argument, nullptr, 'U'},
        {"kohonen", no_argument, nullptr, 'k'},
        {nullptr, 0, nullptr, 0}
    };

    while ((option = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
    	switch(option) {
    		case 'a': {
    			get_available_categories = true;
    			break;
    		}
    		case 'h': {
                with_title = true;
                break;
            }
            case 't': {
                with_text = true;
                break;
            }
            case 'c': {
            	char* new_cluster = strtok(&optarg[0], ",");
            	clusters_to_remove.push_back(string(new_cluster));
            	while (true) {
            		new_cluster = strtok(nullptr, ",");
            		if (new_cluster != nullptr) {
            			clusters_to_remove.push_back(string(new_cluster));
            		}
            		else {
            			break;
            		}
            	}
            	delete new_cluster;
            	break;
            }
			case 'l': {
				char* new_lexical_class = strtok(&optarg[0], ",");
				lexical_classes_to_remove.push_back(string(new_lexical_class));
				while (true) {
					new_lexical_class = strtok(nullptr, ",");
					if (new_lexical_class != nullptr) {
						lexical_classes_to_remove.push_back(string(new_lexical_class));
					}
					else {
						break;
					}
				}
				delete new_lexical_class;
				break;
			}
			case 's': {
				ifstream stop_words_file;
				string stop_word;
				stop_words_file.open(optarg);
				if (stop_words_file.is_open()) {
					while (getline(stop_words_file, stop_word)) {
						stop_words.push_back(stop_word);
					}
					stop_words_file.close();
				}
				break;
			}
			case 'k': {
				do_kohonen_clustering = true;
				break;
			}
			case 'T': {
				category = optarg;
				break;
			}
			case 'F': {
				file_name = optarg;
				break;
			}
			case 'C': {
				clusters = atoi(optarg);
				break;
			}
			case 'S': {
				set_stop_word_list = true;
				stop_words_file_name = optarg;
				break;
			}
			case 'U': {
				upload_data = true;
				upload_data_file_name = optarg;
				break;
			}
		}
	}

	config->get_general_params();
    config->get_servers_params();
    config->get_directories_params();

	trend_process* proc = new trend_process(config);

	cout << "Start program ..." << endl;
	if (get_available_categories == true) {
		proc->get_available_categories();
	}
	if (with_title == true or with_text == true) {
		if (category != "") {
			// Заполняем словарь словами из текстов выбранной тематики
			proc->start_fill_db_with_words(category, with_title, with_text, FROM_DB);
		}
		else if (file_name != "") {
			proc->start_fill_db_with_words(file_name, with_title, with_text, FROM_FILE);
		}
		else {
			cout << "It has not been selected any category or csv file" << endl;
		}
	}
	if (clusters > 0) {
				proc->start_clustering(clusters);
	}
	if (clusters_to_remove.size() > 0) {
		cout << "Remove clusters: " << "| ";

		for (auto &cluster: clusters_to_remove) {
			cout << cluster << " | ";
		}
		cout << endl;
		proc->remove_word_clusters(clusters_to_remove);
	}
	if (lexical_classes_to_remove.size() > 0) {
		cout << "Remove lexical classes: " << "| ";
		for (auto &lexical_class: lexical_classes_to_remove) {
			cout << lexical_class << " | ";
		}
		cout << endl;
		proc->remove_parts_of_speech(lexical_classes_to_remove);
	}
	if (stop_words.size() > 0) {
		cout << "Remove stop words ..." << endl;
		proc->remove_stop_words(stop_words);
	}
	if (set_stop_word_list == true) {
		std::cout << "Set stop word file: " << stop_words_file_name << std::endl;
		proc->set_stop_word_list(stop_words_file_name);
	}
	if (upload_data == true) {
		std::cout << "Upload data to file: " << upload_data_file_name << std::endl;
		proc->uploading_data(upload_data_file_name);
	}
	if (do_kohonen_clustering == true) {
		std::cout << "Start kohonen clustering: " << std::endl;
		proc->do_kohonen_network();
	}
	cout << "Bye!" << endl;
	delete config;
	return 0;
}