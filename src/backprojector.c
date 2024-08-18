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
#include <math.h>       // sinl, cosl
#include <float.h>      // DBL_MAX, DBL_MIN
#include <assert.h>     // assert
#include <unistd.h>     // isatty
#include <omp.h>

#include "backprojector.h" // All of the constants and structs needed for the backprojection algorithm
#include "fileReader.h"    // Functions to read the projection images from the file
#include "fileWriter.h"    // Functions to write the reconstructed 3D object to a file


// Convenient variables accessible by indexing using the axis enum
static const double voxelSize[3] = {VOXEL_SIZE_X, VOXEL_SIZE_Y, VOXEL_SIZE_Z};
static const int nVoxels[3] = {NVOXELS_X, NVOXELS_Y, NVOXELS_Z};
static const int nPlanes[3] = {NPLANES_X, NPLANES_Y, NPLANES_Z};

// Cache the sin and cos values of the angles to avoid recalculating them
long double sinTable[NTHETA], cosTable[NTHETA];


/*******************************************
* Functions to calculate 3D space positions
********************************************/

// Calculate the source position in 3D space
point3D getSourcePosition(int index) {
    return (point3D) {
        .x = -sinTable[index] * DOS,
        .y = cosTable[index] * DOS,
        .z = 0 // 0 because the source is perpendicular to the center of the detector
    };
}

// Calculate a pixel's position in 3D space
point3D getPixelPosition(int row, int col, const projection* projection) {
    // This is the distance from the center of the detector to the top-left pixel's center
    // it's used to calculate the center position of subsequent pixels
    const double dFirstPixel = projection->nSidePixels * PIXEL_SIZE / 2 - PIXEL_SIZE / 2;
    const double sinAngle = sinTable[projection->index];
    const double cosAngle = cosTable[projection->index];

    return (point3D) {
        .x = (DOD * sinAngle) + cosAngle * (-dFirstPixel + col * PIXEL_SIZE),
        .y = -DOD * cosAngle + sinAngle * (-dFirstPixel + col * PIXEL_SIZE),
        .z = -dFirstPixel + row * PIXEL_SIZE
    };
}

/*
* Given a ray's source and the pixel's center, determine if the ray is parallel
* to one of the 3D space axes then return said axis or NONE if not parallel to any.
*/
axis getParallelAxis(const ray ray) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;
    if (source.x == pixel.x) {
        return X;
    } else if (source.y == pixel.y) {
        return Y;
    } else if (source.z == pixel.z) {
        return Z;
    }
    return NONE;
}

double getPlanePosition(const int index, const axis axis) {
    return -((voxelSize[axis] * nVoxels[axis]) / 2) + index * voxelSize[axis];
}

/*********************************************
* Functions defined in the Siddon's algorithm
**********************************************/

void getSidesIntersections(const ray ray, const axis parallelTo, double intersections[3][2]) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;
    for (axis axis = X; axis <= Z; axis++) {
        if (axis == parallelTo) {
            continue; // Skip the axis that the ray is parallel to
        }

        double firstPlane, lastPlane;
        firstPlane = getPlanePosition(0, axis);
        lastPlane = getPlanePosition(nVoxels[axis], axis);

        // Calculate the entry and exit points of the ray with the planes of this axis
        // Siddon's algorithm, equation (4)
        intersections[axis][0] = (firstPlane - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
        intersections[axis][1] = (lastPlane - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
    }
}

double getAMin(const axis parallelTo, double intersections[3][2]) {
    double aMin = 0.0;
    for (axis axis = X; axis <= Z; axis++) {
        if (axis == parallelTo) {
            continue; // Skip the axis that the ray is parallel to
        }

        // Siddon's algorithm, equation (5)
        aMin = fmax(aMin, fmin(intersections[axis][0], intersections[axis][1]));
    }
    return aMin;
}

double getAMax(const axis parallelTo, double intersections[3][2]) {
    double aMax = 1.0;
    for (axis axis = X; axis <= Z; axis++) {
        if (axis == parallelTo) {
            continue; // Skip the axis that the ray is parallel to
        }

        // Siddon's algorithm, equation (5)
        aMax = fmin(aMax, fmax(intersections[axis][0], intersections[axis][1]));
    }
    return aMax;
}

void getPlanesRanges(const ray ray, range planesIndexes[3], const double aMin, const double aMax, const axis parallelTo) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;
    for (axis axis = X; axis <= Z; axis++) {
        double firstPlane = getPlanePosition(0, axis);
        double lastPlane = getPlanePosition(nVoxels[axis], axis);

        // Siddon's algorithm, equation (6)
        int minIndex, maxIndex;
        if (pixel.coordinates[axis] - source.coordinates[axis] >= 0) {
            minIndex = nPlanes[axis] - ceil((lastPlane - aMin * (pixel.coordinates[axis] - source.coordinates[axis]) - source.coordinates[axis]) / voxelSize[axis]);
            maxIndex = floor((source.coordinates[axis] + aMax * (pixel.coordinates[axis] - source.coordinates[axis]) - firstPlane) / voxelSize[axis]);
        } else {
            minIndex = nPlanes[axis] - ceil((lastPlane - aMax * (pixel.coordinates[axis] - source.coordinates[axis]) - source.coordinates[axis]) / voxelSize[axis]);
            // TODO: verify if '1 +' is needed at the beginning of the next line
            maxIndex = floor((source.coordinates[axis] + aMin * (pixel.coordinates[axis] - source.coordinates[axis]) - firstPlane) / voxelSize[axis]);
        }
        planesIndexes[axis] = (range){.min=minIndex, .max=maxIndex};
    }
}

void getAllIntersections(const ray ray, const range planesRanges[3], double* a[3]) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;
    for (axis axis = X; axis <= Z; axis++) {
        int minIndex = planesRanges[axis].min;
        int maxIndex = planesRanges[axis].max;

        if (minIndex >= maxIndex) {
            continue;
        }

        // Siddon's algorithm, equation (7)
        if (pixel.coordinates[axis] - source.coordinates[axis] > 0) {
            a[axis][0] = (getPlanePosition(minIndex, axis) - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
            for (int i = 1; i < maxIndex - minIndex; i++) {
                a[axis][i] = a[axis][i - 1] + voxelSize[axis] / (pixel.coordinates[axis] - source.coordinates[axis]);
            }
        } else if (pixel.coordinates[axis] - source.coordinates[axis] < 0) {
            a[axis][0] = (getPlanePosition(maxIndex, axis) - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
            for (int i = 1; i < maxIndex - minIndex; i++) {
                // TODO: check if '+' or '-' is correct here (it's '+' in Siddon's algorithm, but '-' seems to be correct)
                a[axis][i] = a[axis][i - 1] - voxelSize[axis] / (pixel.coordinates[axis] - source.coordinates[axis]);
            }
        }
    }
}

void mergeIntersections(const double aX[], const double aY[], const double aZ[], const int aXSize, const int aYSize, const int aZSize, double aMerged[]) {
    unsigned int i = 0, j = 0, k = 0, l = 0;

    while (i < aXSize && j < aYSize && k < aZSize) {
        if (aX[i] <= aY[j] && aX[i] <= aZ[k]) {
            aMerged[l++] = aX[i++];
        } else if (aY[j] <= aX[i] && aY[j] <= aZ[k]) {
            aMerged[l++] = aY[j++];
        } else {
            aMerged[l++] = aZ[k++];
        }
    }

    while (i < aXSize && j < aYSize) {
        aMerged[l++] = (aX[i] <= aY[j]) ? aX[i++] : aY[j++];
    }

    while (j < aYSize && k < aZSize) {
        aMerged[l++] = (aY[j] <= aZ[k]) ? aY[j++] : aZ[k++];
    }

    while (i < aXSize && k < aZSize) {
        aMerged[l++] = (aX[i] <= aZ[k]) ? aX[i++] : aZ[k++];
    }

    while (i < aXSize) {
        aMerged[l++] = aX[i++];
    }

    while (j < aYSize) {
        aMerged[l++] = aY[j++];
    }

    while (k < aZSize) {
        aMerged[l++] = aZ[k++];
    }
}

// TODO: time this function and see if it's faster than the previous one
void fastMergeIntersections(const double aX[], const double aY[], const double aZ[], const int aXSize, const int aYSize, const int aZSize, double aMerged[]) {
    unsigned int i = 0, j = 0, k = 0, l = 0;

    while (i < aXSize || j < aYSize || k < aZSize) {
        double x = (i < aXSize) ? aX[i] : DBL_MAX;
        double y = (j < aYSize) ? aY[j] : DBL_MAX;
        double z = (k < aZSize) ? aZ[k] : DBL_MAX;

        if (x <= y && x <= z) {
            aMerged[l++] = x;
            i++;
        } else if (y <= x && y <= z) {
            aMerged[l++] = y;
            j++;
        } else {
            aMerged[l++] = z;
            k++;
        }
    }
}

bool isArraySorted(const double array[], int size) {
    for (int i = 1; i < size; i++) {
        if (array[i] < array[i - 1]) {
            return false;
        }
    }
    return true;
}

void computeAbsorption(const ray ray, const double a[], const int lenA, const double pixelAbsorptionValue, volume* volume) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;

    // distance between the source(1) and the pixel(2)
    // Siddon's algorithm, equation (11)
    const double d12 = sqrt(pow(pixel.x - source.x, 2) + pow(pixel.y - source.y, 2) + pow(pixel.z - source.z, 2));
    const double firstPlaneX = getPlanePosition(0, X);
    const double firstPlaneY = getPlanePosition(0, Y);
    const double firstPlaneZ = getPlanePosition(0, Z);

    for(int i = 1; i < lenA; i ++){
        // Siddon's algorithm, equation (10)
        const double segmentLength = d12 * (a[i] - a[i-1]);
        // Siddon's algorithm, equation (13)
        const double aMid = (a[i] + a[i-1]) / 2;

        // Calculate the voxel indices that the ray intersects
        // Siddon's algorithm, equation (12)
        const int voxelX = ((source.x) + aMid * (pixel.x - source.x) - firstPlaneX) / VOXEL_SIZE_X;
        const int voxelY = ((source.y) + aMid * (pixel.y - source.y) - firstPlaneY) / VOXEL_SIZE_Y;
        const int voxelZ = ((source.z) + aMid * (pixel.z - source.z) - firstPlaneZ) / VOXEL_SIZE_Z;

        // Update the value of the voxel given the value of the pixel and the
        // length of the segment that the ray intersects with the voxel
        const double voxelAbsorptionValue = pixelAbsorptionValue * segmentLength;
        const int voxelIndex = voxelX + voxelY * NVOXELS_Y + voxelZ * NVOXELS_Z;

        //printf("voxelIndex: x =%3d/%d, y =%3d/%d, z =%3d/%d => %7d/%d\n", voxelX, NVOXELS_X, voxelY, NVOXELS_Y, voxelZ, NVOXELS_Z, voxelIndex, NVOXELS_X * NVOXELS_Y * NVOXELS_Z);
        assert(voxelX >= 0 && voxelY >= 0 && voxelZ >= 0);
        assert(voxelX <= NVOXELS_X && voxelY <= NVOXELS_Y && voxelZ <= NVOXELS_Z);
        assert(voxelIndex >= 0 && voxelIndex <= NVOXELS_X * NVOXELS_Y * NVOXELS_Z);
        //printf("index: %7d/%d  current: %10.2lf  new: %10.2lf\n", voxelIndex, NVOXELS_X * NVOXELS_Y * NVOXELS_Z, volume->coefficients[voxelIndex], volume->coefficients[voxelIndex] + voxelAbsorptionValue);
        volume->coefficients[voxelIndex] += voxelAbsorptionValue;
        assert(volume->coefficients[voxelIndex] >= 0);
    }
}

void computeBackProjection(volume* volume, const projection* projection) {
    // Check if the arguments are valid
    if (volume == NULL || projection == NULL) {
        printf("Invalid arguments\n");
        return;
    }

    // Calculate sin and cos values for the angle of this projection
    const long double angle = projection->angle;
    const long double rad = angle * M_PI / 180;
    sinTable[projection->index] = sinl(rad);
    cosTable[projection->index] = cosl(rad);

    // Get the source point of this projection
    const point3D source = getSourcePosition(projection->index);

    // Iterate through every pixel of the projection image and calculate the
    // coefficients of the voxels that contribute to the pixel.
    for (int row = 0; row < projection->nSidePixels; row++) {
        for (int col = 0; col < projection->nSidePixels; col++) {
            const point3D pixel = getPixelPosition(row, col, projection);
            const ray ray = {.source=source, .pixel=pixel};

            // Calculate if the ray is parallel to one of the 3D space axes,
            // this will be useful for checks later in the Siddon's algorithm
            const axis parallelTo = getParallelAxis(ray);

            // This array will contain the intersection points of the ray with
            // the planes. For each (3) axis the first element is the entry point
            // of the ray into the first plane of that axis and the second element
            // is the exit point of the ray from the last plane of that axis.
            double intersections[3][2];
            getSidesIntersections(ray, parallelTo, intersections);

            // Find aMin and aMax with intersections with the side planes
            double aMin = getAMin(parallelTo, intersections);
            double aMax = getAMax(parallelTo, intersections);

            if (aMin >= aMax) {
                continue; // The ray doesn't intersect the volume
            }

            /*
            * These are the (min, max) indices of the planes that the ray intersects
            * in the x, y, and z axes. These indices are used to calculate the
            * coefficients of the voxels that the ray intersects.
            */
            range planesRanges[3];
            getPlanesRanges(ray, planesRanges, aMin, aMax, parallelTo);

            // Calculate all of the intersections of the ray with the planes of
            // all axes and merge them into a single array
            // TODO: a volte ci sono indici negativi, verificare perché
            const int aXSize = fmax(0, planesRanges[X].max - planesRanges[X].min);
            const int aYSize = fmax(0, planesRanges[Y].max - planesRanges[Y].min);
            const int aZSize = fmax(0, planesRanges[Z].max - planesRanges[Z].min);
            assert(aXSize >= 0 && aYSize >= 0 && aZSize >= 0);
            double aX[aXSize], aY[aYSize], aZ[aZSize];
            getAllIntersections(ray, planesRanges, (double*[]){aX, aY, aZ});

            // Merge all of the intersections into a single sorted array
            const int mergedSize = aXSize + aYSize + aZSize;
            double aMerged[mergedSize];
            mergeIntersections(aX, aY, aZ, aXSize, aYSize, aZSize, aMerged);
            assert(isArraySorted(aMerged, mergedSize));

            // Calculate the coefficients of the voxels that the ray intersects
            // TODO: finish implementing this properly
            const double pixelAbsorptionValue = projection->pixels[row * projection->nSidePixels + col];
            computeAbsorption(ray, aMerged, mergedSize, pixelAbsorptionValue, volume);
        }
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
        perror("Error opening input file");
        return 1;
    }

    // Do the same with the output file
    FILE* outputFile = (argc >= 3) ? fopen(argv[2], "w") : stdout;
    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(outputFile);
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

    double initialTime = omp_get_wtime();
    double projectionTimes[NTHETA];

    // Read the projection images from the file and compute the backprojection
    while (readPGM(inputFile, &projection)) {
        fprintf(stderr, "Processing projection %d/%d\n", projection.index + 1, NTHETA);

        double projectionTime = omp_get_wtime();

        computeBackProjection(&volume, &projection);

        projectionTimes[projection.index] = omp_get_wtime() - projectionTime;

        fprintf(stderr, "previous took %.3lf seconds\n", projectionTimes[projection.index]);
        fprintf(stderr, "\033[2A");
    }

    double finalTime = omp_get_wtime();
    fprintf(stderr, "Average time per projection: %.3lf seconds\n", (finalTime - initialTime) / NTHETA);
    fprintf(stderr, "Total time for %d projections: %.3lf seconds\n", NTHETA, finalTime - initialTime);

    writeVolume(outputFile, &volume);

    fclose(inputFile);
    fclose(outputFile);
    free(volume.coefficients);
    free(projection.pixels);
    return 0;
}
