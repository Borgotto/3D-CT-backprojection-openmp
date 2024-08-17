/**
* Usage:
*  compile:  gcc -std=c11 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector
*  run:      ./backprojector < CubeWithSphere.pgm
*  convert:  TODO: convert the outputted 3-dimentional array to a 3D image
*/

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // malloc, calloc, free
#include <stdbool.h>    // bool, true, false
#include <string.h>     // strcmp, strstr
#include <math.h>       // sin, cos
#include <float.h>      // DBL_MAX, DBL_MIN
#include <assert.h>     // assert
#include <unistd.h>     // isatty
#include <omp.h>

#include "backprojector.h" // All of the constants and structs needed for the backprojection algorithm
#include "fileReader.h"    // Functions to read the projection images from the file
#include "fileWriter.h"    // Functions to write the reconstructed 3D object to a file


// Precompute the sin and cos values for each angle to save computation time
double sin_table[NTHETA], cos_table[NTHETA];
void init_tables() {
    for(int i = 0; i < NTHETA; i++) {
        sin_table[i] = sin((AP / 2 - i * STEP_ANGLE) * M_PI / 180);
        cos_table[i] = cos((AP / 2 - i * STEP_ANGLE) * M_PI / 180);
    }
}

int main(int argc, char* argv[]) {

    // Check if input file is provided either as an argument or through stdin
    if (argc < 2 && isatty(fileno(stdin))) {
        fprintf(stderr, "No input file provided\n");
        return 1;
    }
    // Open the input file or use stdin
    FILE* inputFile = (argc >= 2) ? fopen(argv[1], "r") : stdin;
    if (inputFile == NULL) {
        perror("Error opening file");
        return 1;
    }


    /*
    * This struct is used to store the calculated coefficients of the voxels
    * using the backprojection algorithm starting from the projection images
    */
    volume volume = {
        .nVoxelsX = NVOXELS_X,
        .nVoxelsY = NVOXELS_Y,
        .nVoxelsZ = NVOXELS_Z,
        .nPlanesX = NPLANES_X,
        .nPlanesY = NPLANES_Y,
        .nPlanesZ = NPLANES_Z,
        // TODO: find a way to split this array into multiple parts because it's too big (1000^3)*8 bytes = 8GB
        // TODO: a possible solution is to split the volume into chunks and process them serially, this reduces the memory usage but increases the runtime
        // TODO: or sacrifice accuracy for memory by using float instead of double
        .coefficients = (double*)calloc(NVOXELS_X * NVOXELS_Y * NVOXELS_Z, sizeof(double))
    };


    /*
    * This struct is used to store the projection image read from the file
    * its values are overwritten each iteration of the loop below to save memory.
    */
    projection projection;

    // For each projection image in the file
    for (int i = 0; i < NTHETA; i++) {
        readPGM(inputFile, i, &projection);
        if (projection.pixels == NULL) {
            break; // No more images to read
        }

        computeBackProjection(&volume, &projection);

        // free the memory allocated by the readPGM function
        // before reading the next image
        free(projection.pixels);
    }

    writeVolume("output.txt", &volume);
    free(volume.coefficients);
    fclose(inputFile);
    return 0;
}
