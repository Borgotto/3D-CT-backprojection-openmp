/**
 * @file backprojector.c
 * @author Emanuele Borghini (emanuele.borghini@studio.unibo.it)
 * @brief Implementation of the backprojection algorithm for CT reconstruction.
 * @date 2024-09
 * @see backprojector.h
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
 * @copyright
 * ```text
 * This file is part of 3D-CT-backprojection-openmp (https://github.com/Borgotto/3D-CT-backprojection-openmp).
 * Copyright (C) 2024 Emanuele Borghini
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *```
 */

#include <stdio.h>      // fprintf
#include <stdlib.h>     // malloc, calloc, free, exit
#include <stdbool.h>    // bool, true, false
#include <ctype.h>      // tolower
#include <string.h>     // strcmp
#include <math.h>       // sinl, cosl, sqrt, ceil, floor, fmax, fmin, fmod
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
        .coords.x = -sinTable[projectionIndex] * DOS,
        .coords.y = cosTable[projectionIndex] * DOS,
        .coords.z = 0 // 0 because the source is perpendicular to the center of the detector
    };
}

point3D getPixelPosition(const projection* projection, const int row, const int col) {
    // This is the distance from the center of the detector to the top-left pixel's center
    // it's used to calculate the center position of subsequent pixels
    const double dFirstPixel = projection->nSidePixels * PIXEL_SIZE / 2 - PIXEL_SIZE / 2;
    const double sinAngle = sinTable[projection->index];
    const double cosAngle = cosTable[projection->index];

    return (point3D) {
        .coords.x =  DOD * sinAngle + cosAngle * (-dFirstPixel + col * PIXEL_SIZE),
        .coords.y = -DOD * cosAngle + sinAngle * (-dFirstPixel + col * PIXEL_SIZE),
        .coords.z = -dFirstPixel + row * PIXEL_SIZE
    };
}

axis getParallelAxis(const ray ray) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;
    if (source.coords.x == pixel.coords.x) {
        return X;
    } else if (source.coords.y == pixel.coords.y) {
        return Y;
    } else if (source.coords.z == pixel.coords.z) {
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
        intersections[axis][0] = (firstPlane[axis] - source.coordsArray[axis]) /
                                (pixel.coordsArray[axis] - source.coordsArray[axis]);
        intersections[axis][1] = (lastPlane[axis] - source.coordsArray[axis]) /
                                (pixel.coordsArray[axis] - source.coordsArray[axis]);
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
        if (pixel.coordsArray[axis] - source.coordsArray[axis] >= 0) {
            minIndex = N_PLANES[axis] - ceil((lastPlane[axis] - aMin *
                (pixel.coordsArray[axis] - source.coordsArray[axis]) -
                source.coordsArray[axis]) / VOXEL_SIZE[axis]);
            maxIndex = floor((source.coordsArray[axis] + aMax *
                (pixel.coordsArray[axis] - source.coordsArray[axis]) -
                firstPlane[axis]) / VOXEL_SIZE[axis]);
        } else {
            minIndex = N_PLANES[axis] - ceil((lastPlane[axis] - aMax *
                (pixel.coordsArray[axis] - source.coordsArray[axis]) -
                source.coordsArray[axis]) / VOXEL_SIZE[axis]);
            maxIndex = floor((source.coordsArray[axis] + aMin *
                (pixel.coordsArray[axis] - source.coordsArray[axis]) -
                firstPlane[axis]) / VOXEL_SIZE[axis]);
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
        assert(pixel.coordsArray[axis] - source.coordsArray[axis] != 0);
        #endif

        // Siddon's algorithm, equation (7)
        if (pixel.coordsArray[axis] - source.coordsArray[axis] > 0) {
            a[axis][0] = (getPlanePosition(axis, minIndex) - source.coordsArray[axis]) /
                        (pixel.coordsArray[axis] - source.coordsArray[axis]);

            for (int i = 1; i < maxIndex - minIndex; i++) {
                a[axis][i] = a[axis][i - 1] + VOXEL_SIZE[axis] /
                    (pixel.coordsArray[axis] - source.coordsArray[axis]);
            }
        } else if (pixel.coordsArray[axis] - source.coordsArray[axis] < 0) {
            a[axis][0] = (getPlanePosition(axis, maxIndex) - source.coordsArray[axis]) /
                        (pixel.coordsArray[axis] - source.coordsArray[axis]);

            for (int i = 1; i < maxIndex - minIndex; i++) {
                a[axis][i] = a[axis][i - 1] - VOXEL_SIZE[axis] /
                    (pixel.coordsArray[axis] - source.coordsArray[axis]);
            }
        }
    }
}

void mergeIntersections(const double aX[], const double aY[], const double aZ[],
                        const int aXSize, const int aYSize, const int aZSize,
                        double aMerged[]) {
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

void computeAbsorption(const ray ray, const double a[], const int lenA,
                        const volume* volume, const projection* projection,
                        const int pixelIndex) {
    const point3D source = ray.source;
    const point3D pixel = ray.pixel;

    // distance between the source and the pixel
    // Siddon's algorithm, equation (11)
    const double dx = pixel.coords.x - source.coords.x;
    const double dy = pixel.coords.y - source.coords.y;
    const double dz = pixel.coords.z - source.coords.z;
    const double d12 = sqrt(dx * dx + dy * dy + dz * dz);

    // TODO: this needs to be optimized, see if it's possible to use SIMD instructions
    for(int i = 1; i < lenA; i ++){
        // Siddon's algorithm, equation (10)
        const double segmentLength = d12 * (a[i] - a[i-1]);
        // Siddon's algorithm, equation (13)
        const double aMid = (a[i] + a[i-1]) / 2;

        // Calculate the voxel indices that the ray intersects
        // Siddon's algorithm, equation (12)
        const int voxelX = (source.coords.x + aMid * dx - firstPlane[X]) / VOXEL_SIZE_X;
        const int voxelY = (source.coords.y + aMid * dy - firstPlane[Y]) / VOXEL_SIZE_Y;
        const int voxelZ = (source.coords.z + aMid * dz - firstPlane[Z]) / VOXEL_SIZE_Z;

        // Update the value of the voxel given the value of the pixel and the
        // length of the segment that the ray intersects with the voxel
        const double normalizedPixelValue = (projection->pixels[pixelIndex] - projection->minVal) /
                                            (projection->maxVal - projection->minVal);
        const double normalizedSegmentLength = segmentLength / (DOD + DOS);
        const double voxelAbsorptionValue = normalizedPixelValue * normalizedSegmentLength;

        const int voxelIndex = voxelY * N_VOXELS_X * N_VOXELS_Z + voxelZ * N_VOXELS_Z + voxelX;

        #ifdef _DEBUG
        assert(normalizedPixelValue >= 0 && normalizedPixelValue <= 1);
        assert(normalizedSegmentLength >= 0 && normalizedSegmentLength <= 1);
        assert(voxelX >= 0 && voxelX <= N_VOXELS_X);
        assert(voxelY >= 0 && voxelY <= N_VOXELS_Y);
        assert(voxelZ >= 0 && voxelZ <= N_VOXELS_Z);
        assert(voxelAbsorptionValue >= 0);
        assert(voxelIndex >= 0 && voxelIndex < N_VOXELS_X * N_VOXELS_Y * N_VOXELS_Z);
        #endif

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
    //#pragma omp parallel for collapse(2) schedule(dynamic)
    for (int row = 0; row < projection->nSidePixels; row++) {
        for (int col = 0; col < projection->nSidePixels; col++) {
            const point3D pixel = getPixelPosition(projection, row, col);
            const ray ray = {.source=source, .pixel=pixel};
            const axis parallelTo = getParallelAxis(ray);

            #ifdef _DEBUG
            // check if the source and pixel coordinates are NaN or infinite
            assert(!isnan(source.coords.x) && !isnan(source.coords.y) && !isnan(source.coords.z));
            assert(!isinf(source.coords.x) && !isinf(source.coords.y) && !isinf(source.coords.z));
            assert(!isnan(pixel.coords.x) && !isnan(pixel.coords.y) && !isnan(pixel.coords.z));
            assert(!isinf(pixel.coords.x) && !isinf(pixel.coords.y) && !isinf(pixel.coords.z));
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
            assert(planesRanges[X].min >= 0 && planesRanges[X].max <= N_PLANES_X);
            assert(planesRanges[Y].min >= 0 && planesRanges[Y].max <= N_PLANES_Y);
            assert(planesRanges[Z].min >= 0 && planesRanges[Z].max <= N_PLANES_Z);
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
    // Open the input file
    if (argc < 2) {
        fprintf(stderr, "Input file not provided\n");
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char* inputFileName = argv[1];
    FILE* inputFile = fopen(inputFileName, "rb");
    if (inputFile == NULL) {
        fprintf(stderr, "Error opening input file\n");
        exit(EXIT_FAILURE);
    }
    // Get the input file extension
    char* inputFileExtension = strrchr(inputFileName, '.');
    // null-terminate the string if NULL
    inputFileExtension = (inputFileExtension == NULL) ? "" : inputFileExtension;
    // and convert it to lowercase
    for (int i = 0; inputFileExtension[i]; i++) {
        inputFileExtension[i] = tolower(inputFileExtension[i]);
    }
    // if the extension isn't ".dat" or ".pgm" then it's invalid
    if (strcmp(inputFileExtension, ".dat") != 0 &&
        strcmp(inputFileExtension, ".pgm") != 0) {
        fprintf(stderr, "Invalid input file format\n");
        fprintf(stderr, "Supported formats: .dat, .pgm\n");
        exit(EXIT_FAILURE);
    }

    // Open the output file
    if (argc < 3) {
        fprintf(stderr, "Output file not provided\n");
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char* outputFileName = argv[2];
    // Make sure the output file isn't the same as the input file
    if (strcmp(inputFileName, outputFileName) == 0) {
        fprintf(stderr, "Output file can't be the same as the input file\n");
        exit(EXIT_FAILURE);
    }
    // Get the output file extension
    char* outputFileExtension = strrchr(outputFileName, '.');
    // null-terminate the string if NULL
    outputFileExtension = (outputFileExtension == NULL) ? "" : outputFileExtension;
    // and convert it to lowercase
    for (int i = 0; outputFileExtension[i]; i++) {
        outputFileExtension[i] = tolower(outputFileExtension[i]);
    }
    // if the extension isn't ".nrrd" or ".raw" then it's invalid
    if (strcmp(outputFileExtension, ".nrrd") != 0 &&
        strcmp(outputFileExtension, ".raw") != 0) {
        fprintf(stderr, "Invalid output file format\n");
        fprintf(stderr, "Supported formats: .nrrd, .raw\n");
        exit(EXIT_FAILURE);
    }
    FILE* outputFile = fopen(outputFileName, "wb");
    if (outputFile == NULL) {
        fprintf(stderr, "Error opening output file\n");
        exit(EXIT_FAILURE);
    }

    volume volume = {
        .nVoxelsX = N_VOXELS_X,
        .nVoxelsY = N_VOXELS_Y,
        .nVoxelsZ = N_VOXELS_Z,
        .voxelSizeX = VOXEL_SIZE_X,
        .voxelSizeY = VOXEL_SIZE_Y,
        .voxelSizeZ = VOXEL_SIZE_Z,
        .coefficients = (double*)calloc(N_VOXELS_X * N_VOXELS_Y * N_VOXELS_Z,
                                        sizeof(double))
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
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < N_THETA; i++) {
        projection projection;
        bool read;

        // File reading has to be done sequentially
        #pragma omp critical
        if (strcmp(inputFileExtension, ".dat") == 0) {
            read = readProjectionDAT(inputFile, &projection,
                                    &width, &height, &minVal, &maxVal);
        } else {
            read = readProjectionPGM(inputFile, &projection,
                                    &width, &height, &minVal, &maxVal);
        }

        // if read is false, it means that the end of the file was reached
        if (read) {
            #pragma omp atomic update
            processedProjections++;
            fprintf(stderr, "Processing projection %d/%d\r",
                    processedProjections, N_THETA);
            computeBackProjection(&projection, &volume);
        }

        free(projection.pixels);
    }

    double finalTime = omp_get_wtime();
    fprintf(stderr, "\nTime taken (%dx%d): %.3lf seconds\n",
            width, height, (finalTime - initialTime));

    // Write the volume to the output file
    bool done = false;
    int loadingBarIndex = 0;
    char* loadingBar = "|/-\\";
    #pragma omp parallel num_threads(2)
    {
        #pragma omp single nowait
        if (strcmp(outputFileExtension, ".nrrd") == 0) {
            done = writeVolumeNRRD(outputFile, &volume);
        } else {
            done = writeVolumeRAW(outputFile, &volume);
        }
        #pragma omp critical
        while (!done) {
            fprintf(stderr, "Writing volume to file.. %c\r",
                    loadingBar[loadingBarIndex]);
            loadingBarIndex = (loadingBarIndex + 1) % 4;
            // TODO: find platform-independent way to sleep
            nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
        }
    }
    if (done) {
        fprintf(stderr, "Writing volume to file.. Done!\n");
    } else {
        fprintf(stderr, "Error writing the volume to the file!\n");
    }

    // Free memory and handles
    fclose(inputFile);
    fclose(outputFile);
    free(volume.coefficients);
    return 0;
}
