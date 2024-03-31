CC = g++
CFLAGS = -std=c++11 -Wall
LDFLAGS = -lboost_system -lboost_filesystem -pthread

# Object files for each source file
OBJECTS = aux/functions.o server.o client.o

# The final executables
EXECUTABLES = server client

all: clean $(EXECUTABLES)

# Rule to compile server
server: server.cpp
	$(CC) $(CFLAGS) -o server server.cpp $(LDFLAGS)

# Rule to compile client
client: client.cpp aux/functions.o
	$(CC) $(CFLAGS) -o client client.cpp aux/functions.o $(LDFLAGS)

# Rule to compile functions.cpp into an object file
aux/functions.o: aux/functions.cpp aux/functions.h
	$(CC) $(CFLAGS) -c aux/functions.cpp -o aux/functions.o

clean:
	rm -f $(EXECUTABLES) $(OBJECTS)
