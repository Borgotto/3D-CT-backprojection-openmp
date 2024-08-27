/**
 * @file fileReader.h
 * @author Emanuele Borghini (emanuele.borghini@studio.unibo.it)
 * @brief `fileReader` module
 * @date 2024-08-21
 * @see fileWriter.h
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
 * @param maxVal pointer to the variable to store/read the maximum value of the pixels
 * @return `true` if the file was read successfully
 * @return `false` if an error occurred while reading the file or during memory allocation
 */
bool readPGM(FILE* file, projection* projection, int* width, int* height, double* maxVal) {
    // If the read pointer is at the beginning of the file
    if (ftell(file) == 0) {
        char fileFormat[3];

        // Make sure it's a PGM file and read its attributes
        if(fscanf(file, "%2s", fileFormat) == EOF ||
           fscanf(file, "%d %d", width, height) == EOF ||
           fscanf(file, "%lf", maxVal) == EOF) {
            return false; // Error reading the file
        }
        if (strcmp(fileFormat, "P2") != 0) {
            fprintf(stderr, "Unsupported PGM format\n");
            fclose(file);
            exit(1);
        }
    }

    // Initialize the projection struct
    projection->nSidePixels = *width;
    projection->maxVal = *maxVal;
    projection->pixels = (double*)malloc((*width) * (*width) * sizeof(double));
    // Check if the memory allocation was successful
    if (projection->pixels == NULL) {
        fprintf(stderr, "Error allocating memory for the projection\n");
        fclose(file);
        exit(1);
    }

    // Skip lines until "# angle:" is found
    char line[200];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '#') {
            continue;
        }
        if (strstr(line, "angle:") != NULL) {
            break;
        }
    }

    // If we've reached the end of the file
    if (feof(file)) {
        return false;
    }

    // Extract the value after "angle:" from the line and store it in the struct
    sscanf(line, "%*s %*s %lf", &projection->angle);

    // Calculate the index of the projection based on the angle
    // The first projection is at angle AP/2, the last one is at AP/2+AP
    projection->index = (int)((projection->angle + 180) / 360 * N_THETA) % N_THETA;

    #ifdef DEBUG
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