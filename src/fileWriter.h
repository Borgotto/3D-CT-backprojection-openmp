/**
 * @file fileWriter.h
 * @author Emanuele Borghini (emanuele.borghini@studio.unibo.it)
 * @brief `fileWriter` module
 * @date 2024-09
 * @see fileReader.h
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
 * @param volume `volume` struct containing the Voxel data to write
 * @return `true` if the file was written successfully
 * @return `false` if an error occurred while writing the file
 */
bool writeVolumeNRRD(FILE* file, volume* volume) {
    // Write the NRRD header
    fprintf(file, "NRRD0005\n");
    fprintf(file, "# Complete NRRD file format specification at:\n");
    fprintf(file, "# http://teem.sourceforge.net/nrrd/format.html\n");
    fprintf(file, "type: double\n");
    fprintf(file, "dimension: 3\n");
    fprintf(file, "sizes: %d %d %d\n",
            volume->nVoxelsX, volume->nVoxelsY, volume->nVoxelsZ);
    fprintf(file, "spacings: %g %g %g\n",
            volume->voxelSizeX, volume->voxelSizeY, volume->voxelSizeZ);
    fprintf(file, "axis mins: %g %g %g\n",
            -volume->nVoxelsX * volume->voxelSizeX / 2,
            -volume->nVoxelsY * volume->voxelSizeY / 2,
            -volume->nVoxelsZ * volume->voxelSizeZ / 2);

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
    for (int i = 0; i < volume->nVoxelsX * volume->nVoxelsY * volume->nVoxelsZ; i++) {
        fprintf(file, "%g ", volume->coefficients[i]);
    }
    #else
    fprintf(file, "encoding: raw\n\n");
    size_t numVoxels = volume->nVoxelsX * volume->nVoxelsY * volume->nVoxelsZ;
    if (fwrite(volume->coefficients, sizeof(double), numVoxels, file) != numVoxels) {
        return false;  // If fwrite doesn't write all the data, return false
    }
    #endif


    return true;
}

/**
 * @brief Write a 3D volume of voxels to a file.
 *
 * @param file handle to the file to write
 * @param volume `volume` struct containing the Voxel data to write
 * @return `true` if the file was written successfully
 * @return `false` if an error occurred while writing the file
 */
bool writeVolumeRAW(FILE* file, volume* volume) {
    // Print the RAW file properties to standard output for reference when opening the file
    fprintf(stdout, "\r                               \n");
    fprintf(stdout, "RAW file properties:            \n");
    fprintf(stdout, "--------------------------------\n");
    fprintf(stdout, "Image type: %ld-bit Real        \n", sizeof(double) * 8);
    fprintf(stdout, "Width: %d pixels                \n", volume->nVoxelsX);
    fprintf(stdout, "Height: %d pixels               \n", volume->nVoxelsY);
    fprintf(stdout, "Offset to first image: 0 bytes  \n");
    fprintf(stdout, "Number of images: %d            \n", volume->nVoxelsZ);
    fprintf(stdout, "Gap between images: 0 bytes     \n");
    fprintf(stdout, "White is zero: ☐              \n");
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
    size_t numVoxels = volume->nVoxelsX * volume->nVoxelsY * volume->nVoxelsZ;
    if (fwrite(volume->coefficients, sizeof(double), numVoxels, file) != numVoxels) {
        return false;  // If fwrite doesn't write all the data, return false
    }
    return true;
}