CC = gcc
CFLAGS = -std=c99 -Wall -Wpedantic -fopenmp
LFLAGS = -lm

TARGET = backprojector
SRC = backprojector.c

# Default target
all: $(TARGET)

# build the target executable
$(TARGET):
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
