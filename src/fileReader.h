#ifndef BACKPROJECTOR_H
typedef struct projection {
    int index;              // index of the projection
    double angle;           // angle from which the projection was taken
    unsigned int length;    // side length of the square image
    double maxVal;          // maximum value a pixel can have
    double* pixels;         // 2D array of length length * length
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
void readPGM(const char* filename, const unsigned int index, projection* projection) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

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
    projection->length = width;
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
    fclose(file);

    // DEBUG: print the matrix to verify
    // for (int x = 0; x < projection->length; x++) {
    //     for (int y = 0; y < projection->length; y++) {
    //         printf("%3d  ", (int)projection->pixels[x * projection->length + y]);
    //     }
    //     printf("\n");
    // }
}