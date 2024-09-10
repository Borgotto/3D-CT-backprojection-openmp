# 3D-CT-backprojection-openmp

## Compile
Compile using gcc:
```bash
gcc -std=c99 -Wall -Wpedantic -fopenmp -o backprojector src/backprojector.c -lm
```
or simply use the provided Makefile:
```bash
make backprojector
```

---

You can specify the format of `.nrrd` output files by defining the following preprocessor macros:
```bash
gcc [...] -D_OUTPUT_FORMAT_ASCII
```
```bash
gcc [...] -D_OUTPUT_FORMAT_BINARY
```
or using the makefile:
```bash
make backprojector OUTPUT=ASCII
```
```bash
make backprojector OUTPUT=BINARY
```
if not specified **binary** will be used **by default**.\
This does not apply to `.raw` image files as they're always written in binary format.


## Run
Run the program with the following command:
```bash
backprojector <input_file> <output_file>
```
where `<input_file>` is the path to the input file (only `.pgm` or `.dat` are accepted)\
and `<output_file>` is the path to the output file (only `.nrrd` or `.raw` are accepted).

## Visualize
To visualize the output `.nrrd` file, you can use [ITK/VTK Viewer](https://github.com/kitware/itk-vtk-viewer) with its accessible progressive web app [here](https://kitware.github.io/itk-vtk-viewer/app/).\
It's possible to view a file by simply dragging and dropping it into the window, or even by providing a link to it.

For example, let's use [this](output/cubeWithSphereCutout-ascii.nrrd) output file.\
By adding the `?fileToLoad=[link]` query parameter to the URL of the web app you can load the file [like so](https://kitware.github.io/itk-vtk-viewer/app/?fileToLoad=https://raw.githubusercontent.com/Borgotto/3D-CT-backprojection-openmp/main/output/cubeWithSphereCutout-ascii.nrrd) without having to download it.

The ITK/VTK Viewer can also be used to compare two output files to verify the correctness of the program.\
An example of the comparison of two output files can be found [here](https://kitware.github.io/itk-vtk-viewer/app/?rotate=false&image=https://raw.githubusercontent.com/Borgotto/3D-CT-backprojection-openmp/main/output/cubeWithSphereCutout-original.nrrd&fixedImage=https://raw.githubusercontent.com/Borgotto/3D-CT-backprojection-openmp/main/output/cubeWithSphereCutout-ascii.nrrd&compare=blend).

---

To visualize the output `.raw` file you can use [ImageJ](https://imagej.net/ij/) or its web-based version [ImageJ.js](https://ij.imjoy.io/).

Simply open the program, under the `File` menu select `Import` and then `Raw...`\
After selecting the file, you will be prompted with a dialog to specify the image parameters, here you will input the values outputted by the program.

Once opened, you may need to adjust the brightness and contrast of the image.\
This can be done by selecting the `Image` menu, then `Adjust` and `Brightness/Contrast...` or by pressing `Ctrl+Shift+C`.

## Profiling
First, compile the program with the `-pg` flag then run it as usual:
```bash
gcc -std=c99 -Wall -Wpedantic -fopenmp -o backprojector src/backprojector.c -lm -pg
backprojector <input_file> <output_file>
```
convert the `gmon.out` file to a human-readable `.svg` format using the following command:
```bash
gprof backprojector | gprof2dot -n0 -e0 | dot -Tsvg -Gbgcolor=transparent -o profiling/snapshots/"$(ls -l ./profiling/snapshots/ | wc -l) - $(date '+%Y-%m-%d %H.%M.%S')".svg
```
you can optionally convert them to `png`:
```bash
convert -background white -alpha remove -alpha off profiling/snapshots/<snapshot>.svg profiling/snapshots/<snapshot>.png
```

The profiling snapshots found in the [profiling/snapshots](profiling/snapshots) directory were generated using [this](profiling/runProfiler.sh) script

## Documentation
To view the documentation, visit the [GitHub pages](https://borgotto.github.io/3D-CT-backprojection-openmp/) or open the [index.html](docs/index.html) file in your browser.

The documentation was generated using [Doxygen 1.12.0](https://www.doxygen.nl/) with this [Doxyfile](docs/build/Doxyfile).
```bash
doxygen Doxyfile
```
there's also a Makefile target for it:
```bash
make doc
```
the generated documentation can be found in the [docs](docs) directory.

## Example files
The example [input files](tests) were generated using a slightly tuned version of the projection analogue of this program, which can be found [here](https://github.com/LorenzoColletta/Parallel-implementation-of-a-tomographic-projection-algorithm).

The [output files](output) are the result of backprojecting the homonymous input files.