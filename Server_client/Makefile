CC = g++
CFLAGS = -std=c++11 -Wall
LDFLAGS = -lboost_system -lboost_filesystem -pthread

# Object files for each source file
OBJECTS = functions.o server.o client.o

# The final executables
EXECUTABLES = server client

all: clean $(EXECUTABLES)

# Rule to compile server
server: server.cpp
	$(CC) $(CFLAGS) -o server server.cpp $(LDFLAGS)

# Rule to compile client
client: client.cpp functions.o
	$(CC) $(CFLAGS) -o client client.cpp functions.o $(LDFLAGS)

# Rule to compile functions.cpp into an object file
functions.o: functions.cpp functions.h
	$(CC) $(CFLAGS) -c functions.cpp -o functions.o

clean:
	rm -f $(EXECUTABLES) $(OBJECTS)
