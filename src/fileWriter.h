#ifndef BACKPROJECTOR_H
    typedef struct volume {
        unsigned int nVoxelsX, nVoxelsY, nVoxelsZ;
        double* coefficients;   // 3D array of length nVoxelsX * nVoxelsY * nVoxelsZ
    } volume;
#endif

/*
* Function to write the reconstructed 3D object to file.
*
* volume: the struct containing the voxel coefficients
* filename: path to the file to write the object to
*/
void writeVolume(FILE* file, volume* vol) {
    // Write the dimensions of the volume to the file
    fprintf(file, "%d %d %d\n", vol->nVoxelsX, vol->nVoxelsY, vol->nVoxelsZ);

    // TODO: Write the voxel coefficients to the file
}
