#ifndef BACKPROJECTOR_H
typedef struct projection {
    unsigned int size;
    double** pixels;
} projection;
#endif

/*
* Function to read a (part of) PGM file containing the projections of the 3D
* object to reconstruct.
*
* filename: path to the PGM file
* index: the file can contain multiple projections, this parameter specifies
*        which one to read (0-indexed)
* returns: a projection struct with the pixels matrix initialized with the values
*          read or NULL if the index is out of bounds
*/
projection readPGM(const char* filename, unsigned int index) {
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
    int width, height, maxVal;
    fscanf(file, "%d %d", &width, &height);
    fscanf(file, "%d", &maxVal);

    unsigned int numberOfProjections = height / width;
    if (index >= numberOfProjections) {
        // Indicate that the index is out of bounds by returning an empty projection
        return (projection){0, NULL};
    }

    // Skip 'width' lines, 'index' times to get to the desired projection
    for (int i = 0; i < width * index; i++) {
        // Read the whole line
        for (int j = 0; j < width; j++) {
            double pixelValue;
            fscanf(file, "%lf", &pixelValue);
        }
    }

    // Initialize the projection struct
    projection projection;
    projection.size = width;
    projection.pixels = (double**)malloc(projection.size * sizeof(double*));
    for (int i = 0; i < projection.size; i++) {
        projection.pixels[i] = (double*)malloc(projection.size * sizeof(double));
    }

    // Read the pixel values and store them in the matrix
    for (int i = 0; i < projection.size; i++) {
        for (int j = 0; j < projection.size; j++) {
            double pixelValue;
            fscanf(file, "%lf", &pixelValue);
            projection.pixels[i][j] = pixelValue;
        }
    }

    fclose(file);
    return projection;
}