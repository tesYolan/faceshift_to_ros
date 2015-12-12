CC=g++


all: fsshift_ros

fsshift_ros: client.o fsbinarystream.o
	$(CC) -L/usr/lib client.o fsbinarystream.o  -lboost_filesystem -lboost_system -lboost_thread -lpthread  -o fsshift_ros
fsbinarystream.o: fsbinarystream.cpp  fsbinarystream.h
	$(CC) -c fsbinarystream.cpp
client.o: client.cpp fsbinarystream.h
	$(CC) -c  client.cpp 
clean: 
	rm *o