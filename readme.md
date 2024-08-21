# 3D-CT-backprojection-openmp

## Compile
Compile using gcc:
```bash
gcc -std=c11 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector
```
or simply use the provided Makefile:
```bash
make backprojector
```

## Run
Run the program with the following command:
```bash
./backprojector <input_file> <output_file>
```
where `<input_file>` is the path to the input `.PGM` file
and `<output_file>` is the path to the output `.txt` file.

## Profiling
First, compile the program with the `-pg` flag then run it as usual:
```bash
gcc -std=c11 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector -pg
./backprojector <input_file> <output_file>
```
convert the `gmon.out` file to a human-readable format using the following command:
```bash
gprof backprojector | gprof2dot -n0 -e0 | dot -Tsvg -Gbgcolor=transparent -o profiling/snapshots/"$(ls -l ./profiling/snapshots/ | wc -l) - $(date '+%Y-%m-%d %H.%M.%S')".svg
```
you can then convert them to `png`:
```bash
convert -background white -alpha remove -alpha off profiling/snapshots/<snapshot>.svg profiling/snapshots/<snapshot>.png
```

The profiling snapshots found in the [profiling/snapshots](profiling/snapshots) directory were generated using [this](profiling/runProfiler.sh) script

## Documentation
To view the documentation, visit the [GitHub pages](https://borgotto.github.io/3D-CT-backprojection-openmp/) or open the [index.html](docs/index.html) file in your browser.

The documentation was generated using [Doxygen 1.12.0](https://www.doxygen.nl/) and this [Doxyfile](docs/build/Doxyfile).
```bash
doxygen Doxyfile
```
there's also a Makefile target for it:
```bash
make doc
```
the generated documentation can be found in the [docs](docs) directory.


