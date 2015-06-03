CXX ?= g++
LIBS = -lpthread -lboost_regex -lboost_system -lboost_timer -licuuc -lconfig++ -lmysqlclient
CFLAGS = -std=c++11 -c -Wall -O3

all: bad_news categ

bad_news: bad_news.o get_parameters.o mysql_connect.o
	$(CXX) $(LIBS) bad_news.o get_parameters.o mysql_connect.o -o bad_news

categ: categ.o get_parameters.o mysql_connect.o
	$(CXX) $(LIBS) categ.o get_parameters.o mysql_connect.o -o categ

bad_news.o: bad_news.cpp
	$(CXX) $(CFLAGS) bad_news.cpp

categ.o: categ.cpp
	$(CXX) $(CFLAGS) categ.cpp

get_parameters.o: ../get_parameters.cpp
	$(CXX) $(CFLAGS) ../get_parameters.cpp

mysql_connect.o: ../mysql_connect.cpp
	$(CXX) $(CFLAGS) ../mysql_connect.cpp

install:
	install ./bad_news /opt/opinion_miner/utilities/bad_news
	install ./categ /opt/opinion_miner/utilities/categ

dist:
	tar -czf connect.tar.gz *.cpp *.h Makefile*

clean:
	rm -rf *.o bad_news categ
