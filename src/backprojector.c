/**
* Usage:
*  compile:  gcc -std=c99 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector
*  run:      ./backprojector < CubeWithSphere.pgm
*  convert:  TODO: convert the outputted 3-dimentional array to a 3D image
*/

#include "backprojector.h"
#include "fileReader.h"
#include "fileWriter.h"


int main() {
    /*
    * This struct is used to store the calculated coefficients of the voxels
    * using the backprojection algorithm starting from the projection images
    */
    volume volume;
    volume.nVoxelsX = 100;
    volume.nVoxelsY = 100;
    volume.nVoxelsZ = 100;
    volume.coefficients = (double*)malloc(volume.nVoxelsX * volume.nVoxelsY * volume.nVoxelsZ * sizeof(double));

    // For each projection image in the file
    for (int i = 0; ; i++) {
        projection projection;
        readPGM("./tests/example_input.pgm", i, &projection);
        if (projection.pixels == NULL) {
            break; // No more images to read
        }

        // TODO: Implement the backprojection algorithm to calculate the coefficients

        // free the memory allocated by the readPGM function
        // before reading the next image
        free(projection.pixels);
    }

    writeVolume("output.txt", &volume);
    free(volume.coefficients);
    return 0;
}
