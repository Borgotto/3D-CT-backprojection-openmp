/**
 * @file fileReader.h
 * @author Emanuele Borghini (emanuele.borghini@studio.unibo.it)
 * @brief `fileReader` module
 * @date 2024-09
 * @see fileWriter.h
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

#ifndef BACKPROJECTOR_H
/**
 * @brief Struct for representing a CT projection.
 */
typedef struct projection {
    /// Index of the projection out of N_THETA
    int index;
    /// Angle from which the projection was taken
    double angle;
    /// Maximum absorption value assumed by the pixels
    double maxVal;
    /// Number of pixels on one side of the detector (square)
    unsigned int nSidePixels;
    /// 2D array of size (nPixels*nPixels) containing the pixel values
    double* pixels;
} projection;
#endif

/**
 * @brief Read a PGM file containing CT projections.
 *
 * @param file handle to the file to read
 * @param projection `projection` struct to store the read data into
 * @param width pointer to the variable to store/read the width of the image
 * @param height pointer to the variable to store/read the height of the image
 * @param minVal pointer to the variable to store/read the minimum value of the pixels
 * @param maxVal pointer to the variable to store/read the maximum value of the pixels
 * @return `true` if the file was read successfully
 * @return `false` if an error occurred while reading the file or during memory allocation
 */
bool readProjectionPGM(FILE* file, projection* projection, int* width, int* height, double* minVal, double* maxVal) {
    // If the read pointer is at the beginning of the file
    if (ftell(file) == 0) {
        char fileFormat[3];

        // Make sure it's a PGM file and read its attributes
        if (fscanf(file, "%2s", fileFormat) == EOF ||
            fscanf(file, "%d %d", width, height) == EOF ||
            fscanf(file, "%lf", maxVal) == EOF) {
            return false; // Error reading the file
        }
        *minVal = 0; // Minimum value is always 0 in PGM files
        if (strcmp(fileFormat, "P2") != 0) {
            fprintf(stderr, "Unsupported PGM format\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        int nProjections = *height / *width;
        if (nProjections != N_THETA) {
            fprintf(stderr, "Number of projections in the file (%d) doesn't match the expected value (%d)\n", nProjections, N_THETA);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    // Initialize the projection struct
    projection->nSidePixels = *width;
    projection->minVal = *minVal;
    projection->maxVal = *maxVal;
    projection->pixels = (double*)malloc((*width) * (*width) * sizeof(double));
    // Check if the memory allocation was successful
    if (projection->pixels == NULL) {
        fprintf(stderr, "Error allocating memory for the projection\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Skip lines until "#" is found
    char line[100];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] == '#') {
            break;
        }
    }

    // Read the angle from the file
    sscanf(&line[1], "%lf", &projection->angle);

    // Normalize the angle to be between [0, 360) degrees
    projection->angle = fmod(projection->angle + 360, 360);

    // Calculate the index of the projection based on the angle
    // The first projection is at angle AP/2, the last one is at AP/2+AP
    projection->index = (int)((projection->angle + 180) / 360 * N_THETA) % N_THETA;

    #ifdef _DEBUG
    assert(projection->angle >= -360 && projection->angle <= 360);
    assert(projection->index >= 0 && projection->index < N_THETA);
    #endif

    // Read the pixel values and store them in the matrix
    double pixelValue;
    for (int x = 0; x < projection->nSidePixels; x++) {
        for (int y = 0; y < projection->nSidePixels; y++) {
            if (fscanf(file, "%lf", &pixelValue) == EOF) {
                return false;
            }
            projection->pixels[x * projection->nSidePixels + y] = pixelValue;
        }
    }

    return true;
}

/**
 * @brief Read a DAT file containing CT projections.
 *
 * Projection's pixel data must be freed after use.
 *
 * @param file handle to the file to read
 * @param projection `projection` struct to store the read data into
 * @param width pointer to the variable to store/read the width of the image
 * @param height pointer to the variable to store/read the height of the image
 * @param minVal pointer to the variable to store/read the minimum value of the pixels
 * @param maxVal pointer to the variable to store/read the maximum value of the pixels
 * @return `true` if the file was read successfully
 * @return `false` if an error occurred while reading the file or during memory allocation
 */
bool readProjectionDAT(FILE* file, projection* projection, int* width, int* height, double* minVal, double* maxVal) {
    // If the read pointer is at the beginning of the file
    if (ftell(file) == 0) {
        int nProjections;
        if (fread(&nProjections, sizeof(int), 1, file) == 0 ||
            fread(width, sizeof(int), 1, file) == 0 ||
            fread(maxVal, sizeof(double), 1, file) == 0 ||
            fread(minVal, sizeof(double), 1, file) == 0) {
            return false; // Error reading attributes
        }
        *height = *width * nProjections;

        if (nProjections != N_THETA) {
            fprintf(stderr, "Number of projections in the file (%d) doesn't match the expected value (%d)\n", nProjections, N_THETA);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    // Initialize the projection struct
    projection->nSidePixels = *width;
    projection->minVal = *minVal;
    projection->maxVal = *maxVal;
    projection->pixels = (double*)malloc((*width) * (*width) * sizeof(double));
    // Check if the memory allocation was successful
    if (projection->pixels == NULL) {
        fprintf(stderr, "Error allocating memory for the projection\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Read the angle from the file
    if (fread(&projection->angle, sizeof(double), 1, file) == 0) {
        return false; // Error reading the angle
    }

    // Normalize the angle to be between [0, 360) degrees
    projection->angle = fmod(projection->angle + 360, 360);

    // Calculate the index of the projection based on the angle
    // The first projection is at angle AP/2, the last one is at AP/2+AP
    projection->index = (int)((projection->angle + 180) / 360 * N_THETA) % N_THETA;

    #ifdef _DEBUG
    assert(projection->angle >= -360 && projection->angle <= 360);
    assert(projection->index >= 0 && projection->index < N_THETA);
    #endif

    // Read the pixel values and store them in the matrix
    double pixelValue;
    for (int x = 0; x < projection->nSidePixels; x++) {
        for (int y = 0; y < projection->nSidePixels; y++) {
            if (fread(&pixelValue, sizeof(double), 1, file) == 0) {
                return false;
            }
            projection->pixels[x * projection->nSidePixels + y] = pixelValue;
        }
    }

    return true;
}