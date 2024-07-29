/**
* Usage:
*  compile:  gcc -std=c99 -Wall -Wpedantic -fopenmp  backprojector.c -lm -o backprojector
*  run:      ./backprojector 0 1 < CubeWithSphere.pgm
*  convert:  TODO: convert the outputted 3-dimentional array to a 3D image
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define VOXEL_X 100             // voxel side along x-axis
#define VOXEL_Y 100             // voxel side along y-axis
#define VOXEL_Z 100             // voxel side along z-axis

#define PIXEL 85                // detector's pixel side lenght

#define AP 90                   // source path angle
#define STEP_ANGLE 15           // angular distance between each source step
#define NTHETA ((int)((AP) / (STEP_ANGLE))) // number of steps


typedef enum {
    X, Y, Z
} axis;

typedef struct {
    double x, y, z;
} point3D;

typedef struct {
    int min, max;
} range;


int main() {
    return 0;
}
