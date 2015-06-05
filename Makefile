CXX ?= g++
LIBS = -lpthread -lboost_regex -lboost_system -lboost_timer -licuuc -lconfig++ -lpq -lmysqlclient
ALIBS = -Lsvm -lsvm
CFLAGS = -std=c++11 -c -Wall -O3
SHARED_LIB_FLAG = -shared -o libsvm.so.2 svm/svm.cpp

all: opinion_miner opinion_minerd

opinion_miner: opinion_miner.o get_parameters.o pgsql_connect.o mysql_connect.o process.o
	$(CXX) $(LIBS) $(ALIBS) opinion_miner.o get_parameters.o pgsql_connect.o mysql_connect.o process.o -o opinion_miner

opinion_minerd: opinion_minerd.o get_parameters.o pgsql_connect.o mysql_connect.o processd.o
	$(CXX) $(LIBS) $(ALIBS) opinion_minerd.o get_parameters.o pgsql_connect.o mysql_connect.o processd.o -o opinion_minerd

lib: svm/svm.cpp
	$(CXX) -shared -o svm/libsvm.so.2 svm/svm.cpp

opinion_miner.o: opinion_miner.cpp
	$(CXX) $(CFLAGS) opinion_miner.cpp

opinion_minerd.o: opinion_minerd.cpp
	$(CXX) $(CFLAGS) opinion_minerd.cpp

get_parameters.o: get_parameters.cpp
	$(CXX) $(CFLAGS) get_parameters.cpp

pgsql_connect.o: pgsql_connect.cpp
	$(CXX) $(CFLAGS) pgsql_connect.cpp

mysql_connect.o: mysql_connect.cpp
	$(CXX) $(CFLAGS) mysql_connect.cpp

process.o: process.cpp
	$(CXX) $(CFLAGS) process.cpp

processd.o: processd.cpp
	$(CXX) $(CFLAGS) processd.cpp

install:
	install ./opinion_miner /opt/opinion_miner
	install ./opinion_minerd /opt/opinion_minerd

dist:
	tar -czf connect.tar.gz *.cpp *.h Makefile* config/*

clean:
	rm -rf *.o opinion_miner opinion_minerd
