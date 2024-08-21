CC = gcc
CFLAGS = -std=c11 -Wall -Wpedantic -fopenmp
LFLAGS = -lm
DEBUGFLAGS = -D_WORK_UNITS=2352 -O0 -g -pg -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls

TARGET = backprojector
SRC = src/$(TARGET).c

all: $(TARGET) doc

$(TARGET):
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LFLAGS) $(DEBUGFLAGS)

doc:
	cd ./doc && ./doxygen -q Doxyfile

clean:
	rm -rf $(TARGET) gmon.out doc/html/ doc/latex/

.PHONY: all $(TARGET) doc
