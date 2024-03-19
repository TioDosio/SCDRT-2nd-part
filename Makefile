CC = g++
CFLAGS = -std=c++11 -Wall
LDFLAGS = -lboost_system -pthread

all: clean server client

server: server.cpp
	$(CC) $(CFLAGS) -o server server.cpp $(LDFLAGS)

client: client.cpp
	$(CC) $(CFLAGS) -o client client.cpp $(LDFLAGS)

clean:
	rm -f server client
