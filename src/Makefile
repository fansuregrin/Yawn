CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

BIN_DIR = ../bin
TARGET = server
OBJS = ./main.cpp\
       ./buffer/buffer.cpp\
	   ./log/log.cpp\
	   ./epoller/epoller.cpp\
	   ./http/httpconn.cpp\
	   ./http/httprequest.cpp\
	   ./http/httpresponse.cpp\
	   ./pool/sqlconnpool.cpp\
	   ./timer/heap_timer.cpp\
	   ./server/webserver.cpp\
	   ./config/config.cpp

all: $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CFLAGS) $(OBJS) -o $(BIN_DIR)/$(TARGET) -pthread -lmysqlclient

clean:
	rm -rf $(BIN_DIR)/$(TARGET)