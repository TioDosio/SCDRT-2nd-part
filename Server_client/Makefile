CC = g++
CFLAGS = -std=c++11 -Wall
LDFLAGS = -lboost_system -lboost_filesystem -pthread

# Object files for each source file
OBJECTS = server.o client.o funcs.o

# The final executables
EXECUTABLES = server client

all: clean $(EXECUTABLES)

# Rule to compile server
server: server.cpp
	$(CC) $(CFLAGS) -o server server.cpp $(LDFLAGS)

# Rule to compile client
client: client.cpp funcs.o
	$(CC) $(CFLAGS) -o client client.cpp funcs.o $(LDFLAGS)

# Rule to compile funcs.cpp into an object file
funcs.o: funcs.cpp funcs.h
	$(CC) $(CFLAGS) -c funcs.cpp

clean:
	rm -f $(EXECUTABLES) $(OBJECTS)
