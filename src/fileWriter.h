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
    // TODO: Write the voxel coefficients to the file
    return true;
}
