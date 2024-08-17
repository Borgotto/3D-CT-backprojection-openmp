# 3D-CT-backprojection-openmp

## Compile
Compile using gcc:
```bash
gcc -std=c11 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector
```
or simply use the provided Makefile:
```bash
make
```

## Run
Run the program with the following command:
```bash
./backprojector <input_file>
```
where `<input_file>` is the path to the input `.PGM` file


## Capture profiling data
First, compile the program with the `-pg` flag and run it as usual:
```bash
gcc -std=c11 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector -pg
./backprojector <input_file>
```
the profiling snapshots found in the [profiling/snapshots](profiling/snapshots) directory were generated using the following command:
```bash
gprof backprojector | gprof2dot -n0 -e0 | dot -Tsvg -Gbgcolor=transparent -o profiling/snapshots/"$(ls -l ./profiling/snapshots/ | wc -l) - $(date '+%Y-%m-%d %H.%M.%S')".svg
```
you can then convert them to png using the following command:
```bash
convert -background white -alpha remove -alpha off profiling/snapshots/<snapshot>.svg profiling/snapshots/<snapshot>.png
```