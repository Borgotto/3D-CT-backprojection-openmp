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
bool writeVolume(FILE* file, volume* vol) {
    fprintf(file, "<?xml version=\"1.0\"?>\n");

    // Set the endianness of the data
    #if !defined(__ORDER_BIG_ENDIAN__) && !defined(__ORDER_LITTLE_ENDIAN__)
        #error "Endianness not defined"
    #elif __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__
        fprintf(file, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\">\n");
    #else
        fprintf(file, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"BigEndian\">\n");
    #endif

    fprintf(file, "  <ImageData WholeExtent=\"0 %d 0 %d 0 %d\" Origin=\"0 0 0\" Spacing=\"%g %g %g\">\n",
            vol->nVoxelsX - 1, vol->nVoxelsY - 1, vol->nVoxelsZ - 1,
            vol->voxelSizeX, vol->voxelSizeY, vol->voxelSizeZ);
    fprintf(file, "  <Piece Extent=\"0 %d 0 %d 0 %d\">\n",
            vol->nVoxelsX - 1, vol->nVoxelsY - 1, vol->nVoxelsZ - 1);
    fprintf(file, "    <PointData Scalars=\"absorption_coefficients\">\n");

    // Write voxel coefficients
    #ifdef _OUTPUT_ASCII
    fprintf(file, "      <DataArray type=\"Float64\" Name=\"absorption_coefficients\" format=\"ascii\">\n        ");
    for (int i = 0; i < vol->nVoxelsX * vol->nVoxelsY * vol->nVoxelsZ; i++) {
        fprintf(file, "%g ", vol->coefficients[i]);
    }
    #else
    fprintf(file, "      <DataArray type=\"Float64\" Name=\"absorption_coefficients\" format=\"binary\">\n        ");
    // TODO: Writing binary data in XML format requires base64 encoding first
    fwrite(vol->coefficients, sizeof(double), vol->nVoxelsX * vol->nVoxelsY * vol->nVoxelsZ, file);
    #endif

    fprintf(file, "\n      </DataArray>\n");
    fprintf(file, "    </PointData>\n");
    fprintf(file, "    <CellData>\n");
    fprintf(file, "    </CellData>\n");
    fprintf(file, "  </Piece>\n");
    fprintf(file, "  </ImageData>\n");
    fprintf(file, "</VTKFile>\n");

    return true;
}
