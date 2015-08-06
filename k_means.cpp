#include "k_means.h"

k_means::k_means (get_parameters* config)
{
	k_means::config = config;
}

k_means::~k_means ()
{

}

result_clusters k_means::clustering (initial_data data, unsigned int clusters_count, int metric_type)
{
	double size_of_double = (pow(2,sizeof(double) * 8.0 - 1) - 1);	// Наибольшее значение которое может храить double
	bool cluster_changed = true;

	k_means::clusters_count = clusters_count;

	int current_cluster = 0;
	unordered_map<int, bool> cluster_change;

	// Гененрируем начальные центроиды
	set_init_centroids(data);

	for (auto &unit: data) {
		cluster_change[unit.first] = true;
		clusters[unit.first] = 0;
	}

	cout << "k-means # data size:\t\t" << data.size() << endl;
	cout << "k-means # clusters count:\t" << clusters_count << endl;

	size_t iteration_count = 0;
	do {
		iteration_count++;
		for (auto &unit: data) {
			double best_metric = size_of_double;
			double current_metric = 0;
			current_cluster = clusters[unit.first];
			for (size_t cluster_id = 1; cluster_id <= clusters_count; cluster_id++) {
				if (metric_type == MANHATTAN) {
					current_metric = manhattan_metric(unit.second, centroids[cluster_id]);
				}
				else if (metric_type == EUCLIDEAN) {
					current_metric = euclidean_metric(unit.second, centroids[cluster_id]);
				}
				else {
					cout << "k-means # Error: An invalid metric (available metrics: MANHATTAN/EUCLIDEAN)" << endl;
				}
				if (current_metric < best_metric) {
					best_metric = current_metric;
					// Записываем в clusters id элемента из входных данных и номер кластера
					clusters[unit.first] = cluster_id;
				}
			}
			if (current_cluster == clusters[unit.first]) {
				cluster_change[unit.first] = false;
			}
			else {
				cluster_change[unit.first] = true;
			}
		}
		for (auto &unit: cluster_change) {
			if (unit.second == true) {
				cluster_changed = true;
				break;
			}
			else {
				cluster_changed = false;
			}
		}
		// Пересчитываем центроиды
		set_new_centroids(data);
	} while (cluster_changed == true);
	cout << "k_means # iteration count:\t" << iteration_count << endl;
	return clusters;
}

// Вспомогательные функции:
int k_means::set_init_centroids (initial_data & data)
{
	size_t cluster_id = 1;
	int rand_id = 0;
	unsigned long int rand_sum = 0;
	unsigned long int size_of_long_int = (pow(2,sizeof(unsigned long int) * 8.0 - 1) - 1);

	std::list<double>::iterator centr_param;

	// Контейнер для суммы квадратов расстояния
	std::unordered_map<int, double> dx2s;
	std::unordered_map<int, double>::iterator dx2s_u;

	// Генерируем первый центроид
	srand((unsigned)time(NULL));
	rand_id = 1 + rand() % data.size();
	centroids[cluster_id] = data[rand_id];
	cluster_id++;

	// Генерируем все остальные центроиды
	do {
		unsigned long int dx2 = 0, sumdx2 = 0;
		unsigned long int shortest_distance = 0;
		// Для каждой точки
		for (auto &unit: data) {
			shortest_distance = size_of_long_int;
			// Считаем расстояние до каждого центроида
			for (size_t centr_id = 1; centr_id <= centroids.size(); centr_id++) {
				centr_param = centroids[centr_id].begin();
				for (auto unit_param = unit.second.begin(); unit_param != unit.second.end(); ++unit_param) {
					dx2 += pow(abs(*unit_param - *centr_param),2);
					++centr_param;
				}
				dx2 = sqrt(dx2);
				// Выбираем кратчайшее расстояние
				if (dx2 < shortest_distance) {
					shortest_distance = dx2;
				}
				dx2 = 0;
			}
			// Сохраняем все кратчайшие расстояния
			dx2s[unit.first] = shortest_distance;
			sumdx2 += shortest_distance;
			shortest_distance = 0;
		}
		rand_sum = (0 + ((double)rand() / RAND_MAX) * 1) * sumdx2;
		sumdx2 = 0;
		dx2s_u = dx2s.begin();

		do {
			sumdx2 += dx2s_u->second;
			rand_id = dx2s_u->first;
			++dx2s_u;
		} while (sumdx2 < rand_sum);
		centroids[cluster_id] = data[rand_id];
		cluster_id++;
		sumdx2 = 0;
	} while (centroids.size() < clusters_count);

	cout << "k_means # Initial centroids selected:" << endl;
	for (auto &centroid: centroids) {
		for (auto &centr_param: centroid.second) {
			cout << centroid.first << " # " << centr_param << endl;
		}
	}

	return 1;
}

int k_means::set_new_centroids (initial_data & data) 
{
	size_t units_count = 0;
	for (auto &centroid: centroids) {
		for (auto &centr_param: centroid.second) {
			centr_param = 0;
			units_count = 0;
			for (auto &unit: data) {
				if (clusters[unit.first] == centroid.first) {
					for (auto &unit_param: unit.second) {
					    centr_param += unit_param;
					    units_count ++;
				    }
				} 
			}
			centr_param = centr_param / units_count;
		}
	}

	cout << "k_means # New centroids calculated:" << endl;
	for (auto &centroid: centroids) {
		for (auto &centr_param: centroid.second) {
			cout << centroid.first << " # " << centr_param << endl;
		}
	}

	return 1;
}

// Метрики:
double k_means::manhattan_metric (list<double> point_1, list<double> point_2) 
{
	double metric = 0;
	list<double>::iterator point_1_coord;
	list<double>::iterator point_2_coord;

	point_1_coord = point_1.begin();
	point_2_coord = point_2.begin();

	if (point_1.size() == point_2.size()) {
		for (size_t i = 0; i < point_1.size(); i++) {
			metric += abs(*point_1_coord - *point_2_coord);
			++point_1_coord;
			++point_2_coord;
		}
	}
	return metric;
}

double k_means::euclidean_metric (list<double> point_1, list<double> point_2)
{
	double metric = 0;
	list<double>::iterator point_1_coord;
	list<double>::iterator point_2_coord;

	point_1_coord = point_1.begin();
	point_2_coord = point_2.begin();

	if (point_1.size() == point_2.size()) {
		for (size_t i = 0; i < point_1.size(); i++) {
			metric	+= pow(*point_1_coord - *point_2_coord, 2);
			++point_1_coord;
			++point_2_coord;
		}
	}
	return sqrt(metric);
}