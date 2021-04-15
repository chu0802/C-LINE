.PHONY: clean
CXX = g++
CXXFLAGS = -std=c++11 -O2
LIBS = -I ./include
SRCS = ./src
TARGET = server client
SERVER_OBJS = server.o protocol.o command_handler.o
CLIENT_OBJS = client.o protocol.o myUI.o client_cmd_handler.o

all: $(TARGET)
	rm -f *.o

%.o: $(SRCS)/%.cpp
	$(CXX) $(CXXFLAGS) $< $(LIBS) -c

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o *.txt $(TARGET) 
