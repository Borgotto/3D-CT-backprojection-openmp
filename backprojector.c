/**
* Usage:
*  compile:  gcc -std=c99 -Wall -Wpedantic -fopenmp backprojector.c -lm -o backprojector
*  run:      ./backprojector < CubeWithSphere.pgm
*  convert:  TODO: convert the outputted 3-dimentional array to a 3D image
*/

#include "backprojector.h"
#include "fileReader.h"


int main() {
    projection image;
    image.size = -1; // Initialize to a negative value to enter the loop

    for (int i = 0; image.size != 0; i++) {
        image = readPGM("image.pgm", i);

        // Print the matrix to verify
        for (int i = 0; i < image.size; i++) {
            for (int j = 0; j < image.size; j++) {
                printf("%d ", (int)image.pixels[i][j]);
            }
            printf("\n");
        }

        // Free the image's pixels matrix before reading the next one
        for (int i = 0; i < image.size; i++) {
            free(image.pixels[i]);
        }
        free(image.pixels);
    }

    return 0;
}
