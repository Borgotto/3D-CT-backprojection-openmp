CC = gcc
CFLAGS = -g -std=c99 -Wall -Wpedantic -fopenmp
LFLAGS = -lm

TARGET = backprojector
SRC = src/backprojector.c

all: $(TARGET)
.PHONY: $(TARGET) # force the target to be rebuilt
$(TARGET):
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
