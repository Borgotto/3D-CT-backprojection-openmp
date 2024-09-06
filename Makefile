# Input variables
WORK_UNITS ?= 236 # number of work units to process (int)
INPUT ?= BINARY # input file format (ASCII or BINARY) if not specified, BINARY is used
OUTPUT ?= BINARY # output file format (ASCII or BINARY) if not specified, BINARY is used

CC = gcc
CFLAGS = -std=c99 -Wall -Wpedantic -fopenmp -O0 -D_INPUT_FORMAT_$(strip $(INPUT)) -D_OUTPUT_FORMAT_$(strip $(OUTPUT))
LFLAGS = -lm

# Add debug flags if WORK_UNITS is set
ifneq ($(strip $(WORK_UNITS)), 0)
	DEBUGFLAGS = -D_WORK_UNITS=$(strip $(WORK_UNITS)) -D_DEBUG -g -pg -fno-omit-frame-pointer -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls
endif

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
