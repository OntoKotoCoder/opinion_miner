CXX ?= g++
LIBS = -lpthread -lboost_regex -licuuc -lconfig++ -lpq -lmysqlclient
CFLAGS = -std=c++11 -c -Wall

all: connect

connect: main.o get_parameters.o pgsql_connect.o mysql_connect.o process.o
	$(CXX) $(LIBS) main.o get_parameters.o pgsql_connect.o mysql_connect.o process.o -o connect

main.o: main.cpp
	$(CXX) $(CFLAGS) main.cpp

get_parameters.o: get_parameters.cpp
	$(CXX) $(CFLAGS) get_parameters.cpp

pgsql_connect.o: pgsql_connect.cpp
	$(CXX) $(CFLAGS) pgsql_connect.cpp

mysql_connect.o: mysql_connect.cpp
	$(CXX) $(CFLAGS) mysql_connect.cpp

process.o: process.cpp
	$(CXX) $(CFLAGS) process.cpp

install:
	install ./connect /opt/sentiment_analysis

dist:
	tar -czf connect.tar.gz *.cpp *.h Makefile config/*

clean:
	rm -rf *.o
