/**
* Usage:
*  compile:  gcc -std=c99 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector
*  run:      ./backprojector < CubeWithSphere.pgm
*  convert:  TODO: convert the outputted 3-dimentional array to a 3D image
*/

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // malloc, calloc, free
#include <stdbool.h>    // bool, true, false
#include <string.h>     // strcmp, strstr
#include <math.h>       // sin, cos
#include <float.h>      // DBL_MAX, DBL_MIN
#include <omp.h>

#include "backprojector.h"
#include "fileReader.h"
#include "fileWriter.h"


// Precompute the sin and cos values for each angle to save computation time
double sin_table[NTHETA], cos_table[NTHETA];
void init_tables() {
    for(int i = 0; i < NTHETA; i++) {
        sin_table[i] = sin((AP / 2 - i * STEP_ANGLE) * M_PI / 180);
        cos_table[i] = cos((AP / 2 - i * STEP_ANGLE) * M_PI / 180);
    }
}


int main() {
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
