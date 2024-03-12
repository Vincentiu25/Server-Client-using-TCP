CC=g++

CFLAGS=-lstdc++ -std=c++17 -Wall -Wextra -g

all: server subscriber

server: server.cpp
	$(CC) $(CFLAGS) server.cpp utils.cpp -o server

subscriber: subscriber.cpp
	$(CC) $(CFLAGS) subscriber.cpp utils.cpp -o subscriber

clean:
	rm -f server subscriber