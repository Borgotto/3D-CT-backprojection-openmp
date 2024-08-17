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
* index: the file can contain multiple projections, this parameter specifies
*        which one to read (0-indexed)
* projection: the projection struct to store the read image and its attributes
*/
void readPGM(FILE* file, const unsigned int index, projection* projection) {
    // TODO: receive the file pointer as a parameter broke the parser,
    // TODO: as a workaround, seek to the beginning of the file before reading
    fseek(file, 0, SEEK_SET);

    // Make sure it's a PGM file
    char fileFormat[3];
    fscanf(file, "%2s", fileFormat);
    if (strcmp(fileFormat, "P2") != 0) {
        fprintf(stderr, "Unsupported PGM format\n");
        fclose(file);
        exit(1);
    }

    // Read the PGM file attributes
    int width, height;
    double maxVal;
    fscanf(file, "%d %d", &width, &height);
    fscanf(file, "%lf", &maxVal);

    unsigned int numberOfProjections = height / width;
    if (index >= numberOfProjections) {
        // Indicate that the index is out of bounds by returning an empty pixels array
        projection->pixels = NULL;
        return;
    }

    // Skip lines until the desired projection is found
    // TODO: more efficient way to skip lines using fseek
    char line[200];
    for (int i = 0; i <= index; i++) {
        // Skip lines until "# angle:" is found (ignoring whitespaces)
        while (fgets(line, sizeof(line), file) != NULL) {
            if (line[0] != '#') {
                continue;
            }
            if (strstr(line, "angle:") != NULL) {
                break;
            }
        }
    }

    // DEBUG: print the line to verify it's the correct one
    //printf("%s", line);


    // Initialize the projection struct
    projection->index = index;
    sscanf(line, "%*s %*s %lf", &projection->angle);
    projection->nSidePixels = width;
    projection->maxVal = maxVal;
    projection->pixels = (double*)malloc(width * width * sizeof(double));
    // extract the value after "angle:" from the line and store it in the struct

    // Make sure the angle is in the range [-360, 360] inclusive
    if (projection->angle < -360 || projection->angle > 360) {
        fprintf(stderr, "Invalid angle value read: %lf\n", projection->angle);
        fclose(file);
        exit(1);
    }

    // Read the pixel values and store them in the matrix
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < width; y++) {
            double pixelValue;
            fscanf(file, "%lf", &pixelValue);
            projection->pixels[x * width + y] = pixelValue;
        }
    }

    // DEBUG: print the matrix to verify
    // for (int x = 0; x < projection->nSidePixels; x++) {
    //     for (int y = 0; y < projection->nSidePixels; y++) {
    //         printf("%3d  ", (int)projection->pixels[x * projection->nSidePixels + y]);
    //     }
    //     printf("\n");
    // }
}