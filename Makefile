all:Shared-directory ./BakeUpDir/upload
Shared-directory:main.cpp	
	g++ -g -std=c++11 $^ -o $@ -lpthread -lboost_system -lboost_filesystem
./BakeUpDir/upload:upload.cpp 
	g++ -g -std=c++11 $^ -o $@ -lpthread -lboost_system -lboost_filesystem
