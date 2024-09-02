/**
 * @file backprojector.c
 * @author Emanuele Borghini (emanuele.borghini@studio.unibo.it)
 * @date 2024
 *
 * @brief Implementation of the backprojection algorithm for CT reconstruction.
 *
 * @see backprojector.h
 *
 * @details
 * This file contains the implementation of the backprojection algorithm for
 * reconstructing a 3D object from its 2D projections. The algorithm is based
 * on Siddon's algorithm and is parallelized using OpenMP.
 * The algorithm reads the projection images from a file and writes the
 * reconstructed 3D object to another file.
 *
 * The algorithm is divided into several functions, each of which implements
 * an equation from Siddon's algorithm. The main function reads the projection
 * images from the file and computes the backprojection for each of them.
 *
 */

#include <stdio.h>      // fprintf
#include <stdlib.h>     // malloc, calloc, free
#include <stdbool.h>    // bool, true, false
#include <string.h>     // strcmp, strstr
#include <math.h>       // sinl, cosl, sqrt, ceil, floor, fmax, fmin
#include <unistd.h>     // isatty
#include <time.h>       // nanosleep
#include <omp.h>        // omp_get_wtime, #pragma omp
#ifdef _DEBUG
#include <assert.h>     // assert
#endif

#include "backprojector.h" // All of the constants and structs needed for the backprojection algorithm
#include "fileReader.h"    // Functions to read the projection images from the file
#include "fileWriter.h"    // Functions to write the reconstructed 3D object to a file


// Cache the sin and cos values of the angles to avoid recalculating them
long double sinTable[N_THETA], cosTable[N_THETA];

// Cache the first and last plane positions for each axis
double firstPlane[3], lastPlane[3];

void initTables() {
    // Calculate sin and cos values for the angle of this projection
    for (int i = 0; i < N_THETA; i++) {
        const long double angle = AP / 2 + i * STEP_ANGLE;
        const long double rad = angle * M_PI / 180;
        sinTable[i] = sinl(rad);
        cosTable[i] = cosl(rad);
    }

    // Calculate the first and last plane positions for each axis
    for (axis axis = X; axis <= Z; axis++) {
        firstPlane[axis] = -(VOXEL_SIZE[axis] * N_VOXELS[axis]) / 2;
        lastPlane[axis] = -firstPlane[axis];
    }
}

point3D getSourcePosition(const int projectionIndex) {
    return (point3D) {
        .x = -sinTable[projectionIndex] * DOS,
        .y = cosTable[projectionIndex] * DOS,
        .z = 0 // 0 because the source is perpendicular to the center of the detector
    };
}

point3D getPixelPosition(const projection* projection, const int row, const int col) {
    // This is the distance from the center of the detector to the top-left pixel's center
    // it's used to calculate the center position of subsequent pixels
    const double dFirstPixel = projection->nSidePixels * PIXEL_SIZE / 2 - PIXEL_SIZE / 2;
    const double sinAngle = sinTable[projection->index];
    const double cosAngle = cosTable[projection->index];

    return (point3D) {
        .x =  DOD * sinAngle + cosAngle * (-dFirstPixel + col * PIXEL_SIZE),
        .y = -DOD * cosAngle + sinAngle * (-dFirstPixel + col * PIXEL_SIZE),
        .z = -dFirstPixel + row * PIXEL_SIZE
    };
}

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

double getPlanePosition(const axis axis, const int index) {
    // Siddon's algorithm, equation (3)
    return firstPlane[axis] + index * VOXEL_SIZE[axis];
}

void getSidesIntersections(const ray ray, const axis parallelTo, double intersections[3][2]) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;
    for (axis axis = X; axis <= Z; axis++) {
        if (axis == parallelTo) {
            continue; // Skip the axis that the ray is parallel to
        }

        // Calculate the entry and exit points of the ray with the planes of this axis
        // Siddon's algorithm, equation (4)
        intersections[axis][0] = (firstPlane[axis] - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
        intersections[axis][1] = (lastPlane[axis] - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
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

void getPlanesRanges(const ray ray, range planesRanges[3], const double aMin, const double aMax) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;
    for (axis axis = X; axis <= Z; axis++) {
        // Siddon's algorithm, equation (6)
        int minIndex, maxIndex;
        if (pixel.coordinates[axis] - source.coordinates[axis] >= 0) {
            minIndex = N_PLANES[axis] - ceil((lastPlane[axis] - aMin * (pixel.coordinates[axis] - source.coordinates[axis]) - source.coordinates[axis]) / VOXEL_SIZE[axis]);
            maxIndex = floor((source.coordinates[axis] + aMax * (pixel.coordinates[axis] - source.coordinates[axis]) - firstPlane[axis]) / VOXEL_SIZE[axis]);
        } else {
            minIndex = N_PLANES[axis] - ceil((lastPlane[axis] - aMax * (pixel.coordinates[axis] - source.coordinates[axis]) - source.coordinates[axis]) / VOXEL_SIZE[axis]);
            maxIndex = floor((source.coordinates[axis] + aMin * (pixel.coordinates[axis] - source.coordinates[axis]) - firstPlane[axis]) / VOXEL_SIZE[axis]);
        }
        planesRanges[axis] = (range){.min=minIndex, .max=maxIndex};
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

        #ifdef _DEBUG
        assert(pixel.coordinates[axis] - source.coordinates[axis] != 0);
        #endif

        // Siddon's algorithm, equation (7)
        if (pixel.coordinates[axis] - source.coordinates[axis] > 0) {
            a[axis][0] = (getPlanePosition(axis, minIndex) - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
            for (int i = 1; i < maxIndex - minIndex; i++) {
                a[axis][i] = a[axis][i - 1] + VOXEL_SIZE[axis] / (pixel.coordinates[axis] - source.coordinates[axis]);
            }
        } else if (pixel.coordinates[axis] - source.coordinates[axis] < 0) {
            a[axis][0] = (getPlanePosition(axis, maxIndex) - source.coordinates[axis]) / (pixel.coordinates[axis] - source.coordinates[axis]);
            for (int i = 1; i < maxIndex - minIndex; i++) {
                a[axis][i] = a[axis][i - 1] - VOXEL_SIZE[axis] / (pixel.coordinates[axis] - source.coordinates[axis]);
            }
        }
    }
}

void mergeIntersections(const double aX[], const double aY[], const double aZ[], const int aXSize, const int aYSize, const int aZSize, double aMerged[]) {
    int i = 0, j = 0, k = 0, l = 0;

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

#ifdef _DEBUG
bool isArraySorted(const double array[], int size) {
    for (int i = 1; i < size; i++) {
        if (array[i] < array[i - 1]) {
            return false;
        }
    }
    return true;
}
#endif

void computeAbsorption(const ray ray, const double a[], const int lenA, const volume* volume, const projection* projection, const int pixelIndex) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;

    // distance between the source and the pixel
    // Siddon's algorithm, equation (11)
    const double dx = pixel.x - source.x;
    const double dy = pixel.y - source.y;
    const double dz = pixel.z - source.z;
    const double d12 = sqrt(dx * dx + dy * dy + dz * dz);

    // TODO: this needs to be optimized, see if it's possible to use SIMD instructions
    for(int i = 1; i < lenA; i ++){
        // Siddon's algorithm, equation (10)
        const double segmentLength = d12 * (a[i] - a[i-1]);
        // Siddon's algorithm, equation (13)
        const double aMid = (a[i] + a[i-1]) / 2;

        // Calculate the voxel indices that the ray intersects
        // Siddon's algorithm, equation (12)
        const int voxelX = (source.x + aMid * (pixel.x - source.x) - firstPlane[X]) / VOXEL_SIZE_X;
        const int voxelY = (source.y + aMid * (pixel.y - source.y) - firstPlane[Y]) / VOXEL_SIZE_Y;
        const int voxelZ = (source.z + aMid * (pixel.z - source.z) - firstPlane[Z]) / VOXEL_SIZE_Z;

        // Update the value of the voxel given the value of the pixel and the
        // length of the segment that the ray intersects with the voxel
        const double normalizedPixelValue = (projection->pixels[pixelIndex] - projection->minVal) / (projection->maxVal - projection->minVal);
        const double normalizedSegmentLength = segmentLength / (DOD + DOS);
        const double voxelAbsorptionValue = normalizedPixelValue * normalizedSegmentLength;

        const int voxelIndex = voxelY * N_VOXELS_X * N_VOXELS_Z + voxelZ * N_VOXELS_Z + voxelX;

        #ifdef _DEBUG
        assert(normalizedPixelValue >= 0 && normalizedPixelValue <= 1);
        assert(normalizedSegmentLength >= 0 && normalizedSegmentLength <= 1);
        assert(voxelX >= 0 && voxelY >= 0 && voxelZ >= 0);
        assert(voxelX <= N_VOXELS_X && voxelY <= N_VOXELS_Y && voxelZ <= N_VOXELS_Z);
        assert(voxelAbsorptionValue >= 0);
        assert(voxelIndex >= 0 && voxelIndex <= N_VOXELS_X * N_VOXELS_Y * N_VOXELS_Z);
        #pragma omp critical
        assert(volume->coefficients[voxelIndex] + voxelAbsorptionValue >= volume->coefficients[voxelIndex]);
        #endif

        // TODO: find a way to avoid using atomic operations
        // Siddon's algorithm, equation (14)
        #pragma omp atomic update
        volume->coefficients[voxelIndex] += voxelAbsorptionValue;
    }
}

void computeBackProjection(const projection* projection, volume* volume) {
    // Check if the arguments are valid
    if (volume == NULL || projection == NULL) {
        fprintf(stderr, "Invalid arguments\n");
        exit(EXIT_FAILURE);
    }

    // Get the source point of this projection
    const point3D source = getSourcePosition(projection->index);

    // Iterate through every pixel of the projection image and calculate the
    // coefficients of the voxels that contribute to the pixel.
    //#pragma omp parallel for collapse(2) schedule(dynamic) default(none) shared(volume, projection, source)
    // TODO: to reduce the atomic operations, it may be possible to offset the pixel processing by a certain amount * thread_id
    // TODO: so that each thread shoots rays at different pixels, this could reduce the intersecting voxels between threads
    for (int row = 0; row < projection->nSidePixels; row++) {
        for (int col = 0; col < projection->nSidePixels; col++) {
            const point3D pixel = getPixelPosition(projection, row, col);
            const ray ray = {.source=source, .pixel=pixel};
            const axis parallelTo = getParallelAxis(ray);

            #ifdef _DEBUG
            // check if the source and pixel coordinates are NaN or infinite
            assert(!isnan(source.x) && !isnan(source.y) && !isnan(source.z));
            assert(!isnan(pixel.x) && !isnan(pixel.y) && !isnan(pixel.z));
            assert(!isinf(source.x) && !isinf(source.y) && !isinf(source.z));
            assert(!isinf(pixel.x) && !isinf(pixel.y) && !isinf(pixel.z));
            #endif

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
            getPlanesRanges(ray, planesRanges, aMin, aMax);

            #ifdef _DEBUG
            assert(planesRanges[X].min >= 0 && planesRanges[Y].min >= 0 && planesRanges[Z].min >= 0);
            assert(planesRanges[X].max <= N_PLANES_X && planesRanges[Y].max <= N_PLANES_Y && planesRanges[Z].max <= N_PLANES_Z);
            #endif

            // Calculate all of the intersections of the ray with the planes of
            // all axes and merge them into a single array
            const int aXSize = fmax(0, planesRanges[X].max - planesRanges[X].min);
            const int aYSize = fmax(0, planesRanges[Y].max - planesRanges[Y].min);
            const int aZSize = fmax(0, planesRanges[Z].max - planesRanges[Z].min);
            double aX[aXSize], aY[aYSize], aZ[aZSize];
            getAllIntersections(ray, planesRanges, (double*[]){aX, aY, aZ});

            // Calculate the size of the merged array
            // Siddon's algorithm, equation (9)
            const int mergedSize = aXSize + aYSize + aZSize;
            double aMerged[mergedSize];

            // Merge all of the intersections into a single sorted array
            // Siddon's algorithm, equation (8)
            mergeIntersections(aX, aY, aZ, aXSize, aYSize, aZSize, aMerged);
            #ifdef _DEBUG
            assert(isArraySorted(aX, aXSize));
            assert(isArraySorted(aY, aYSize));
            assert(isArraySorted(aZ, aZSize));
            assert(isArraySorted(aMerged, mergedSize));
            #endif

            // Calculate the coefficients of the voxels that the ray intersects
            const int pixelIndex = row * projection->nSidePixels + col;
            computeAbsorption(ray, aMerged, mergedSize, volume, projection, pixelIndex);
        }
    }
}


int main(int argc, char* argv[]) {
    // Check if input file is provided either as an argument or through stdin
    if (argc < 2 && isatty(fileno(stdin))) { // TODO: find alternative to isatty
        fprintf(stderr, "No input file provided\n");
        exit(EXIT_FAILURE);
    }

    // Open the input file or use stdin
    FILE* inputFile = (argc >= 2) ? fopen(argv[1], "rb") : stdin;
    if (inputFile == NULL) {
        fprintf(stderr, "Error opening input file\n");
        exit(EXIT_FAILURE);
    }

    // Do the same with the output file
    FILE* outputFile = (argc >= 3) ? fopen(argv[2], "wb") : stdout;
    if (outputFile == NULL) {
        fprintf(stderr, "Error opening output file\n");
        exit(EXIT_FAILURE);
    }


    /*
    * This struct is used to store the calculated coefficients of the voxels
    * using the backprojection algorithm starting from the projection images
    */
    volume volume = {
        .nVoxelsX = N_VOXELS_X,
        .nVoxelsY = N_VOXELS_Y,
        .nVoxelsZ = N_VOXELS_Z,
        .voxelSizeX = VOXEL_SIZE_X,
        .voxelSizeY = VOXEL_SIZE_Y,
        .voxelSizeZ = VOXEL_SIZE_Z,
        // TODO: find a way to split this array into multiple parts because it's too big (1000^3)*8 bytes = 8GB
        // TODO: a possible solution is to split the volume into chunks and process them serially, this reduces the memory usage but increases the runtime
        // TODO: or sacrifice accuracy for memory by using float instead of double
        .coefficients = (double*)calloc(N_VOXELS_X * N_VOXELS_Y * N_VOXELS_Z, sizeof(double))
    };
    // Check if the memory was allocated successfully
    if (volume.coefficients == NULL) {
        fprintf(stderr, "Error allocating memory for the volume\n");
        exit(EXIT_FAILURE);
    }

    initTables();

    double initialTime = omp_get_wtime();

    // Projection attributes to be read from file
    int width, height;
    double minVal, maxVal;

    // Read the projection images from the file and compute the backprojection
    int processedProjections = 0;
    #pragma omp parallel for schedule(dynamic) default(none) shared(inputFile, volume, width, height, minVal, maxVal, stderr, processedProjections)
    for (int i = 0; i < N_THETA; i++) {
        projection projection;
        bool read;

        // TODO: find a way to not have to pass the width, height, minVal, and maxVal to the function
        // TODO: fix projection index assignment, it shouldn't depend on the projection angle
        // File reading has to be done sequentially
        #pragma omp critical
        read = readProjection(inputFile, &projection, &width, &height, &minVal, &maxVal);

        // if read is false, it means that the end of the file was reached
        if (read) {
            #pragma omp atomic update
            processedProjections++;
            fprintf(stderr, "Processing projection %d/%d\r", processedProjections, N_THETA);
            computeBackProjection(&projection, &volume);
        }

        free(projection.pixels);
    }

    double finalTime = omp_get_wtime();
    fprintf(stderr, "\nTime taken (%dx%d): %.3lf seconds\n", width, height, (finalTime - initialTime));

    // Write the volume to the output file
    bool done = false;
    int loadingBarIndex = 0;
    char* loadingBar = "|/-\\";
    #pragma omp parallel num_threads(2) default(none) shared(outputFile, volume, done, loadingBarIndex, loadingBar, stderr)
    {
        #pragma omp single nowait
        done = writeVolume(outputFile, &volume);
        #pragma omp critical
        while (!done) {
            fprintf(stderr, "Writing volume to file.. %c\r", loadingBar[loadingBarIndex]);
            loadingBarIndex = (loadingBarIndex + 1) % 4;
            // TODO: find platform-independent way to sleep
            nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
        }
    }
    fprintf(stderr, "Writing volume to file.. Done!\n");

    // Free memory and handles
    fclose(inputFile);
    fclose(outputFile);
    free(volume.coefficients);
    return 0;
}
