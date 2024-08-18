#ifndef BACKPROJECTOR_H
    typedef struct projection {
        int index;                   // index of the projection out of NTHETA
        double angle;                // angle from which the projection was taken
        double maxVal;               // maximum absorption value assumed by the pixels
        unsigned int nSidePixels;    // numbers of pixels on one side of the detector (square)
        double* pixels;              // 2D array of size nPixels * nPixels
    } projection;
#endif

/*
* Function to read a (part of) PGM file containing the projections of the 3D
* object to reconstruct.
*
* filename: path to the PGM file
* projection: the projection struct to store the read image and its attributes
* return: true if the file was read successfully, false if the end of the file was reached
*/
bool readPGM(FILE* file, projection* projection) {
    // If the read pointer is at the beginning of the file
    if (ftell(file) == 0) {
        char fileFormat[3];
        int width, height;
        double maxVal;

        // Make sure it's a PGM file and read its attributes
        if(fscanf(file, "%2s", fileFormat) == EOF ||
           fscanf(file, "%d %d", &width, &height) == EOF ||
           fscanf(file, "%lf", &maxVal) == EOF) {
            return false; // Error reading the file
        }
        if (strcmp(fileFormat, "P2") != 0) {
            fprintf(stderr, "Unsupported PGM format\n");
            fclose(file);
            exit(1);
        }
        // Initialize the projection struct
        projection->nSidePixels = width;
        projection->maxVal = maxVal;
        projection->pixels = (double*)malloc(width * width * sizeof(double));
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
    // The first projection is at angle AP-AP/2, the last one is at AP+AP/2
    projection->index = (int)((projection->angle - AP / 2) / STEP_ANGLE);

    // Make sure the angle is in the range [-360, 360] inclusive
    if (projection->angle < -360 || projection->angle > 360) {
        fprintf(stderr, "Invalid angle value read: %lf\n", projection->angle);
        fclose(file);
        exit(1);
    }

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