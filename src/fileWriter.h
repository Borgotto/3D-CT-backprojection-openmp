/**
 * @file fileWriter.h
 * @author Emanuele Borghini (emanuele.borghini@studio.unibo.it)
 * @brief `fileWriter` module
 * @date 2024-08-21
 * @see fileReader.h
 */

#ifndef BACKPROJECTOR_H
/**
 * @brief Struct for representing a 3D volume of voxels.
 */
typedef struct volume {
    /// Number of voxels in the x-axis
    const unsigned int nVoxelsX;
    /// Number of voxels in the y-axis
    const unsigned int nVoxelsY;
    /// Number of voxels in the z-axis
    const unsigned int nVoxelsZ;
    /// Number of planes in the x-axis
    const unsigned int nPlanesX;
    /// Number of planes in the y-axis
    const unsigned int nPlanesY;
    /// Number of planes in the z-axis
    const unsigned int nPlanesZ;
    /// 3D array of size (nVoxelsX*nVoxelsY*nVoxelsZ) containing the absorption coefficients
    double* coefficients;
} volume;
#endif

/**
 * @brief Write a 3D volume of voxels to a file.
 *
 * @param file handle to the file to write
 * @param vol `volume` struct containing the Voxel data to write
 * @return `true` if the file was written successfully
 * @return `false` if an error occurred while writing the file
 */
bool writeVolumeNRRD(FILE* file, volume* vol) {
    // Write the NRRD header
    fprintf(file, "NRRD0005\n");
    fprintf(file, "# Complete NRRD file format specification at:\n");
    fprintf(file, "# http://teem.sourceforge.net/nrrd/format.html\n");
    fprintf(file, "type: double\n");
    fprintf(file, "dimension: 3\n");
    fprintf(file, "sizes: %d %d %d\n", vol->nVoxelsX, vol->nVoxelsY, vol->nVoxelsZ);
    fprintf(file, "spacings: %g %g %g\n", vol->voxelSizeX, vol->voxelSizeY, vol->voxelSizeZ);
    fprintf(file, "axis mins: %g %g %g\n", -vol->nVoxelsX * vol->voxelSizeX / 2, -vol->nVoxelsY * vol->voxelSizeY / 2, -vol->nVoxelsZ * vol->voxelSizeZ / 2);

    // Set the endianness of the data
    #if !defined(__ORDER_BIG_ENDIAN__) && !defined(__ORDER_LITTLE_ENDIAN__)
        #error "Endianness not defined"
    #elif __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__
        fprintf(file, "endian: little\n");
    #else
        fprintf(file, "endian: big\n");
    #endif

    // Set the encoding of the data and write the coefficients
    #ifdef _OUTPUT_FORMAT_ASCII
    fprintf(file, "encoding: ascii\n\n");
    for (int i = 0; i < vol->nVoxelsX * vol->nVoxelsY * vol->nVoxelsZ; i++) {
        fprintf(file, "%g ", vol->coefficients[i]);
    }
    #else
    fprintf(file, "encoding: raw\n\n");
    size_t numVoxels = vol->nVoxelsX * vol->nVoxelsY * vol->nVoxelsZ;
    if (fwrite(vol->coefficients, sizeof(double), numVoxels, file) != numVoxels) {
        return false;  // If fwrite doesn't write all the data, return false
    }
    #endif


    return true;
}

/**
 * @brief Write a 3D volume of voxels to a file.
 *
 * @param file handle to the file to write
 * @param vol `volume` struct containing the Voxel data to write
 * @return `true` if the file was written successfully
 * @return `false` if an error occurred while writing the file
 */
bool writeVolumeRAW(FILE* file, volume* vol) {
    // Print the RAW file properties to standard output for reference when opening the file
    fprintf(stdout, "\r                               \n");
    fprintf(stdout, "RAW file properties:            \n");
    fprintf(stdout, "--------------------------------\n");
    fprintf(stdout, "Image type: %ld-bit Real        \n", sizeof(double) * 8);
    fprintf(stdout, "Width: %d pixels                \n", vol->nVoxelsX);
    fprintf(stdout, "Height: %d pixels               \n", vol->nVoxelsY);
    fprintf(stdout, "Offset to first image: 0 bytes  \n");
    fprintf(stdout, "Number of images: %d            \n", vol->nVoxelsZ);
    fprintf(stdout, "Gap between images: 0 bytes     \n");
    fprintf(stdout, "White is zero: 🗹              \n");
    #if !defined(__ORDER_BIG_ENDIAN__) && !defined(__ORDER_LITTLE_ENDIAN__)
        #error "Endianness not defined"
    #elif __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__
    fprintf(stdout, "Little-endian byte order: 🗹   \n");
    #else
    fprintf(stdout, "Little-endian byte order: ☐    \n");
    #endif
    fprintf(stdout, "Open all files in folder: ☐    \n");
    fprintf(stdout, "Use virtual stacks: ☐          \n");
    fprintf(stdout, "--------------------------------\n\n");

    // Write the coefficients to the file
    size_t numVoxels = vol->nVoxelsX * vol->nVoxelsY * vol->nVoxelsZ;
    if (fwrite(vol->coefficients, sizeof(double), numVoxels, file) != numVoxels) {
        return false;  // If fwrite doesn't write all the data, return false
    }
    return true;
}