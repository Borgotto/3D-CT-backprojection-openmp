#ifndef BACKPROJECTOR_H
#define BACKPROJECTOR_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define VOXEL_X 100             // size of a single voxel in the x-axis
#define VOXEL_Y 100             // size of a single voxel in the y-axis
#define VOXEL_Z 100             // size of a single voxel in the z-axis

#define PIXEL 85                // side lenght of a single square pixel

#define AP 90                   // rays source initial angle (in degrees)
#define STEP_ANGLE 15           // distance between rays sources (in degrees)
#define NTHETA ((int)((AP) / (STEP_ANGLE))) // number of steps


typedef enum axis {
    X, Y, Z
} axis;

typedef struct point3D {
    double x, y, z;
} point3D;

typedef struct range {
    int min, max;
} range;

typedef struct projection {
    unsigned int size;
    double** pixels;
} projection;