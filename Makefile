Shared-directory:main.cpp	
	g++ -g -std=c++11 $^ -o $@ -lpthread -lboost_system -lboost_filesystem
