CC = g++
LIBS_FLAGS = -pthread -Wall -lrt -lSimpleAmqpClient #-lpigpio
SRC = main.cpp src/*
TARGET = build/main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $^ -o $@ $(LIBS_FLAGS) 

clean:
	rm -f $(TARGET)