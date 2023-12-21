# Makefile for server.c

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wno-unused-variable

# Target executable
TARGET = server

# Source file
SOURCE = server.c

all: $(TARGET) run

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
