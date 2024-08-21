CC = gcc
CFLAGS = -std=c11 -Wall -Wpedantic -fopenmp
LFLAGS = -lm
DEBUGFLAGS = -D_WORK_UNITS=2352 -O0 -g -pg -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls

TARGET = backprojector
SRC = src/backprojector.c

all: $(TARGET)
.PHONY: $(TARGET) # force the target to be rebuilt
$(TARGET):
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LFLAGS) $(DEBUGFLAGS)

clean:
	rm -f $(TARGET) gmon.out

.PHONY: all clean
