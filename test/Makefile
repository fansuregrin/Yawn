CXX = g++
CFLAGS = -std=c++14 -Wall

BIN_DIR = ../bin
TARGET = yawn_test
OBJS = ./main.cpp\
	   ./config_unittest.cc\
       ../src/buffer/buffer.cpp\
	   ../src/log/log.cpp\
	   ../src/config/config.cpp\

all: $(OBJS)
	mkdir -p $(BIN_DIR)
	cp ./test_server.cfg $(BIN_DIR)
	$(CXX) $(CFLAGS) $(OBJS) -o $(BIN_DIR)/$(TARGET) -lgtest -pthread

clean:
	rm -rf $(BIN_DIR)/$(TARGET)