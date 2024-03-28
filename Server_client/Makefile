CC = g++
CFLAGS = -std=c++11 -Wall
LDFLAGS = -lboost_system -lboost_filesystem -pthread

# Object files for each source file
OBJECTS = aux/funcs.o server.o client.o

# The final executables
EXECUTABLES = server client

all: clean $(EXECUTABLES)

# Rule to compile server
server: server.cpp
	$(CC) $(CFLAGS) -o server server.cpp $(LDFLAGS)

# Rule to compile client
client: client.cpp aux/funcs.o
	$(CC) $(CFLAGS) -o client client.cpp aux/funcs.o $(LDFLAGS)

# Rule to compile funcs.cpp into an object file
aux/funcs.o: aux/funcs.cpp aux/funcs.h
	$(CC) $(CFLAGS) -c aux/funcs.cpp -o aux/funcs.o

clean:
	rm -f $(EXECUTABLES) $(OBJECTS)
