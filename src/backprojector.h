#ifndef BACKPROJECTOR_H
    #define BACKPROJECTOR_H
#endif

#ifndef M_PI
    #define M_PI (3.14159265358979323846)
#endif

//! All of these physical constants are measured in micrometers unless stated otherwise
//! The origin of the 3D space is at the volumetric center of the object

#define VOXEL_SIZE_X 100         // size of a single voxel in the x-axis in
#define VOXEL_SIZE_Y 100         // size of a single voxel in the y-axis in
#define VOXEL_SIZE_Z 100         // size of a single voxel in the z-axis in
#define PIXEL_SIZE   85          // side lenght of a single square pixel

#define AP 90                    // rays source initial angle (in degrees)
#define STEP_ANGLE 15            // distance between rays sources (in degrees)

#ifdef _WORK_UNITS
    // These values will be used when running benchmarks for scalability
    #define VOXEL_MATRIX_SIZE ((int)((_WORK_UNITS) * (VOXEL_SIZE_X) * 125 / 294))
    #define DOD ((int)(1.5 * (VOXEL_MATRIX_SIZE)))
    #define DOS ((int)(6 * (VOXEL_MATRIX_SIZE)))
#else
    #define VOXEL_MATRIX_SIZE 100000 // side length of the volumetric object (cube)
    #define DOD 150000               // distance from the volumetric center of object to the detector
    #define DOS 600000               // distance from the volumetric center of object to the source
#endif

//! These two values depend on the input data and are calculated at runtime
//#define DETECTOR_SIZE     200000 // side length of the detector (square)
//#define NPIXELS   ((int)((DETECTOR_SIZE) / (PIXEL_SIZE)))   // number of pixels in the detector

// Macros for derived constants
#define NVOXELS_X ((int)(VOXEL_MATRIX_SIZE / VOXEL_SIZE_X)) // number of voxels in the x-axis
#define NVOXELS_Y ((int)(VOXEL_MATRIX_SIZE / VOXEL_SIZE_Y)) // number of voxels in the y-axis
#define NVOXELS_Z ((int)(VOXEL_MATRIX_SIZE / VOXEL_SIZE_Z)) // number of voxels in the z-axis

#define NPLANES_X ((NVOXELS_X) + 1)                         // number of planes in the x-axis
#define NPLANES_Y ((NVOXELS_Y) + 1)                         // number of planes in the y-axis
#define NPLANES_Z ((NVOXELS_Z) + 1)                         // number of planes in the z-axis

#define NTHETA    ((int)((AP) / (STEP_ANGLE)) + 1)          // number of rays sources

typedef enum axis {
    NONE = -1,
    X = 0,
    Y = 1,
    Z = 2
} axis;

typedef union point3D {
    double coordinates[3];
    struct {
        double x, y, z;
    };
} point3D;

typedef union range {
    int bounds[2];
    struct {
        int min, max;
    };
} range;

typedef struct projection {
    int index;                   // index of the projection out of NTHETA
    double angle;                // angle from which the projection was taken
    double maxVal;               // maximum absorption value assumed by the pixels
    unsigned int nSidePixels;    // numbers of pixels on one side of the detector (square)
    double* pixels;              // 2D array of size nPixels * nPixels
} projection;

typedef struct volume {
    const unsigned int nVoxelsX, nVoxelsY, nVoxelsZ; // equal to NVOXELS_X, NVOXELS_Y, NVOXELS_Z
    const unsigned int nPlanesX, nPlanesY, nPlanesZ; // equal to NPLANES_X, NPLANES_Y, NPLANES_Z
    double* coefficients;        // 3D array of size nVoxelsX * nVoxelsY * nVoxelsZ
} volume;