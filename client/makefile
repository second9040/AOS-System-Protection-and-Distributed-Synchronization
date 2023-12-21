# Makefile for client.c

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wno-unused-variable

# Target executable
TARGET = client

# Source file
SOURCE = client.c

all: $(TARGET) run

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

run:
	./$(TARGET)

clean:
	rm -f $(TARGET)
