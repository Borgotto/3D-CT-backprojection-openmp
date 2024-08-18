#!/bin/env python3

# This script is meant to convert a pgm file outputted from projector.c
# to a format that can be read by backprojector.c

import sys

# take input file and output file as arguments
input_file = next(iter(sys.argv[1:]), None) or "tests/input.pgm"
output_file = next(iter(sys.argv[2:]), None) or "tests/output.pgm"

# take angle and step as arguments, if not provided, use default values
angle = next(iter(sys.argv[3:]), None) or 45
step = next(iter(sys.argv[4:]), None) or 15

# open the input pgm file
with open(input_file, 'r') as infile:
    lines = infile.readlines()

# read the pgm header then data
header = []
data = []
is_header = True
for line in lines:
    if is_header:
        header.append(line)
        if line.strip().isdigit():
            is_header = False
    else:
        data.append(line)

# get the width of the image
width = int(header[1].split()[0])

# write the output pgm file
with open(output_file, 'w') as outfile:
    # write the header unchanged
    for line in header:
        outfile.write(line)

    # every width lines, add a comment with the angle
    for i, line in enumerate(data):
        if i % width == 0:
            outfile.write(f"# angle: {angle + i // width * step}°\n")
        outfile.write(line)
