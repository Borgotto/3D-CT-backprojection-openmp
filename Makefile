CC = gcc
CFLAGS = -std=c11 -Wall -Wpedantic -fopenmp
LFLAGS = -lm
# pass `WORK_UNITS='-D_WORK_UNITS='` to 'make backprojector' to specify work units
DEBUGFLAGS = $(WORK_UNITS) -D_DEBUG -O0 -g -pg -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls

TARGET = backprojector
SRC = src/$(TARGET).c

all: $(TARGET) doc

$(TARGET):
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LFLAGS) $(DEBUGFLAGS)

doc:
	cd ./docs/build && ./doxygen -q Doxyfile

clean:
	# binary and profiling data
	rm -f $(TARGET) gmon.out
	# delete documentation files while keeping the directories
	find docs/ -maxdepth 1 -type f -delete

.PHONY: all $(TARGET) doc
