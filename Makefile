CC = gcc
CFLAGS = -std=c11 -Wall -Wpedantic -fopenmp -g -pg
LFLAGS = -lm -D_WORK_UNITS=20

TARGET = backprojector
SRC = src/backprojector.c

all: $(TARGET)
.PHONY: $(TARGET) # force the target to be rebuilt
$(TARGET):
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LFLAGS)

clean:
	rm -f $(TARGET) gmon.out

.PHONY: all clean
