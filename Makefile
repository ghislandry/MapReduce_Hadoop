CC = g++
HADOOP_INSTALL = /usr/local/hadoop
PLATFORM = Linux-amd64-64
CPPFLAGS = -I$(HADOOP_INSTALL)/c++/Linux-amd64-64/include/hadoop

simtags: MapReduceTest.cpp
	$(CC) $(CPPFLAGS) $< -Wall -L$(HADOOP_INSTALL)/c++/Linux-amd64-64/lib/ -lhadooppipes \
	-lhadooputils -lpthread -lcrypto -lssl -g -O2 -o $@

