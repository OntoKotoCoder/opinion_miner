#include <iostream>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <list>
#include <cmath>

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "get_parameters.h"

using namespace std;
using namespace boost;

enum metric_types {MANHATTAN, EUCLIDEAN};

// <id-слова, список параметров>
typedef unordered_map<int, list<double>> initial_data;
// <id-кластера, список параметров>
typedef unordered_map<int, list<double>> new_centroids;
// <id-слова, id-кластера>
typedef unordered_map<int, int> result_clusters;

class k_means
{

public:
	new_centroids centroids;
	result_clusters clusters;

	k_means (get_parameters* config);
	~k_means ();

	// Кластеризация k-means
	result_clusters clustering (initial_data data, unsigned int clusters_count, int metric_type);

private:
	// Параметры:
	get_parameters* config;
	unsigned int clusters_count;
	// Вспомогательные функции
	int set_init_centroids (initial_data & data);
	int set_new_centroids (initial_data & data);

	// Метрики:
	double manhattan_metric (list<double> point_1, list<double> point_2);
	double euclidean_metric (list<double> point_1, list<double> point_2);
};