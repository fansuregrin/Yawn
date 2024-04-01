CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

BIN_DIR = ../bin
TARGET = test
OBJS = ./test.cpp\
	   ./test_log.cpp\
	   ./test_timer.cpp\
	   ./test_config.cpp\
       ../src/buffer/buffer.cpp\
	   ../src/log/log.cpp\
	   ../src/config/config.cpp\
	   ../src/timer/heap_timer.cpp

all: $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CFLAGS) $(OBJS) -o $(BIN_DIR)/$(TARGET)

clean:
	rm -rf $(BIN_DIR)/$(TARGET)