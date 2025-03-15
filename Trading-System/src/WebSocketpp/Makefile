# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I./src -I./src/Custom_WebSocket -I/usr/include/openssl

# Required libraries
LDFLAGS = -lssl -lcrypto -lboost_system -lpthread

# Target executable
TARGET = algo.exe

# Source directories
SRC_DIR = src
TEST_DIR = test

# Object files for the main project
SRC_OBJECTS = $(SRC_DIR)/main.o $(SRC_DIR)/Trader.o $(SRC_DIR)/Api.o $(SRC_DIR)/Socket.o $(SRC_DIR)/BSocket.o $(SRC_DIR)/Socketpp.o \
              $(SRC_DIR)/Boost_WebSocket/BSocket.o $(SRC_DIR)/Custom_WebSocket/CSocket.o $(SRC_DIR)/Custom_WebSocket/CParser.o 

# Object files for testing
TEST_OBJECTS = $(TEST_DIR)/test_latency.o $(TEST_DIR)/test_throughput.o

# Default target: Compile everything
all: $(TARGET)

# Link object files to create the main executable
$(TARGET): $(SRC_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(SRC_OBJECTS) $(TEST_OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile each source file into an object file
$(SRC_DIR)/main.o: $(SRC_DIR)/main.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(SRC_DIR)/main.o

$(SRC_DIR)/Trader.o: $(SRC_DIR)/Trader.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/Trader.cpp -o $(SRC_DIR)/Trader.o

$(SRC_DIR)/Api.o: $(SRC_DIR)/Api.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/Api.cpp -o $(SRC_DIR)/Api.o

$(SRC_DIR)/Socket.o: $(SRC_DIR)/Socket.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/Socket.cpp -o $(SRC_DIR)/Socket.o

$(SRC_DIR)/BSocket.o: $(SRC_DIR)/BSocket.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/BSocket.cpp -o $(SRC_DIR)/BSocket.o

$(SRC_DIR)/Socketpp.o: $(SRC_DIR)/Socketpp.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/Socketpp.cpp -o $(SRC_DIR)/Socketpp.o

$(SRC_DIR)/Boost_WebSocket/BSocket.o: $(SRC_DIR)/Boost_WebSocket/BSocket.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/Boost_WebSocket/BSocket.cpp -o $(SRC_DIR)/Boost_WebSocket/BSocket.o

$(SRC_DIR)/Custom_WebSocket/CSocket.o: $(SRC_DIR)/Custom_WebSocket/CSocket.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/Custom_WebSocket/CSocket.cpp -o $(SRC_DIR)/Custom_WebSocket/CSocket.o

$(SRC_DIR)/Custom_WebSocket/CParser.o: $(SRC_DIR)/Custom_WebSocket/CParser.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/Custom_WebSocket/CParser.cpp -o $(SRC_DIR)/Custom_WebSocket/CParser.o

# Compile test files
$(TEST_DIR)/test_latency.o: $(TEST_DIR)/test_latency.cpp
	$(CXX) $(CXXFLAGS) -c $(TEST_DIR)/test_latency.cpp -o $(TEST_DIR)/test_latency.o

$(TEST_DIR)/test_throughput.o: $(TEST_DIR)/test_throughput.cpp
	$(CXX) $(CXXFLAGS) -c $(TEST_DIR)/test_throughput.cpp -o $(TEST_DIR)/test_throughput.o

# Clean up all compiled files
clean:
	rm -f $(SRC_OBJECTS) $(TEST_OBJECTS) $(TARGET)

# Phony targets
.PHONY: all clean