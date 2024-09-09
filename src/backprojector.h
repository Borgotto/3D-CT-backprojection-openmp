/**
 * @file backprojector.h
 * @author Emanuele Borghini (emanuele.borghini@studio.unibo.it)
 * @brief Header file for the backprojector module.
 * @date 2024-09
 * @see backprojector.c
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
    #define BACKPROJECTOR_H
#endif

#ifndef M_PI
    /// Define 𝝅(PI) if not included in the math library
    #define M_PI (3.14159265358979323846)
#endif

/// Size of a single voxel in the X axis (in micrometers)
#define VOXEL_SIZE_X 100
/// size of a single voxel in the y-axis (in micrometers)
#define VOXEL_SIZE_Y 100
/// size of a single voxel in the z-axis (in micrometers)
#define VOXEL_SIZE_Z 100
/// side lenght of a single square pixel (in micrometers)
#define PIXEL_SIZE   85

/// rays source initial angle (in degrees)
#define AP 360
/// distance between rays sources (in degrees)
#define STEP_ANGLE 15

#if defined(_WORK_UNITS) && _WORK_UNITS > 0
    // These values will be used when running benchmarks for scalability
    #define VOXEL_MATRIX_SIZE ((int)((_WORK_UNITS) * (VOXEL_SIZE_X) * 125 / 294))
    #define DOD ((int)(1.5 * (VOXEL_MATRIX_SIZE)))
    #define DOS ((int)(6 * (VOXEL_MATRIX_SIZE)))
#else
    // These values can be modified based on the machine's capabilities

    /// side length of the volumetric object (cube)
    #define VOXEL_MATRIX_SIZE 100000
    /// distance from the volumetric center of object to the detector
    #define DOD 150000
    /// distance from the volumetric center of object to the source
    #define DOS 600000
#endif

// ! These two values depend on the input data and are calculated at runtime
//#define DETECTOR_SIZE     200000 // side length of the detector (square)
//#define NPIXELS   ((int)((DETECTOR_SIZE) / (PIXEL_SIZE)))   // number of pixels in the detector

// Macros for derived constants

/// number of voxels in the x-axis
#define N_VOXELS_X ((int)(VOXEL_MATRIX_SIZE / VOXEL_SIZE_X))
/// number of voxels in the y-axis
#define N_VOXELS_Y ((int)(VOXEL_MATRIX_SIZE / VOXEL_SIZE_Y))
/// number of voxels in the z-axis
#define N_VOXELS_Z ((int)(VOXEL_MATRIX_SIZE / VOXEL_SIZE_Z))

/// number of planes in the x-axis
#define N_PLANES_X ((N_VOXELS_X) + 1)
/// number of planes in the y-axis
#define N_PLANES_Y ((N_VOXELS_Y) + 1)
/// number of planes in the z-axis
#define N_PLANES_Z ((N_VOXELS_Z) + 1)

/// number of rays sources
#define N_THETA    ((int)((AP) / (STEP_ANGLE)) + 1)

// Convenience variable accessible by indexing using the axis enum
static const double VOXEL_SIZE[3] = {VOXEL_SIZE_X, VOXEL_SIZE_Y, VOXEL_SIZE_Z};
static const int N_VOXELS[3] = {N_VOXELS_X, N_VOXELS_Y, N_VOXELS_Z};
static const int N_PLANES[3] = {N_PLANES_X, N_PLANES_Y, N_PLANES_Z};

/**
 * @brief Enum for representing the axes of 3D space.
 */
typedef enum axis {
    /// No axis
    NONE = -1,
    /// X axis
    X = 0,
    /// Y axis
    Y = 1,
    /// Z axis
    Z = 2
} axis;

/**
 * @brief Union of Struct/Array for representing a 3D point in space.
 */
typedef union point3D {
    /// Array form of coordinates struct
    const double coordsArray[3];
    struct coords {
        /// X coordinate
        const double x;
        /// Y coordinate
        const double y;
        /// Z coordinate
        const double z;
    } coords;
} point3D;

/**
 * @brief Struct for representing a straight line in 3D space.
 */
typedef struct ray {
    /// Source (starting) point of the ray
    const point3D source;
    /// Pixel (ending) point of the ray
    const point3D pixel;
} ray;

/**
 * @brief Struct for representing a range of @c int
 *        with minimum and maximum bounds.
 */
typedef struct range {
    /// Minimum bound
    int min;
    /// Maximum bound
    int max;
} range;

/**
 * @brief Struct for representing a CT projection.
 */
typedef struct projection {
    /// Index of the projection out of N_THETA
    int index;
    /// Angle from which the projection was taken
    double angle;
    /// Minimum absorption value assumed by the pixels
    double minVal;
    /// Maximum absorption value assumed by the pixels
    double maxVal;
    /// Number of pixels on one side of the detector (square)
    int nSidePixels;
    /// 2D array of size (nPixels*nPixels) containing the pixel values
    double* pixels;
} projection;

/**
 * @brief Struct for representing a 3D volume of voxels.
 */
typedef struct volume {
    /// Number of voxels in the x-axis
    const int nVoxelsX;
    /// Number of voxels in the y-axis
    const int nVoxelsY;
    /// Number of voxels in the z-axis
    const int nVoxelsZ;
    /// Size of voxels in the x-axis
    const double voxelSizeX;
    /// Size of voxels in the y-axis
    const double voxelSizeY;
    /// Size of voxels in the z-axis
    const double voxelSizeZ;
    /// 3D array of size (nVoxelsX*nVoxelsY*nVoxelsZ) containing the absorption coefficients
    double* coefficients;
} volume;


/**
 * @brief Initializes the sine and cosine tables, as well as the firstPlane and lastPlane arrays.
 *
 * These values are complex to calculate each time, so they are precomputed and
 * cached to optimize performance during backprojection operations.
 */
void initTables();

/**
 * @brief Calculates the 3D coordinates of the source of the ray given the index of the projection.
 *
 * The source is located at a distance of DOS from the volumetric center of the object.
 * The angle is derived from the index and the step angle.
 *
 * @param projectionIndex The index of the projection.
 * @return The 3D coordinates of the source.
 */
point3D getSourcePosition(const int projectionIndex);

/**
 * @brief Calculates the 3D coordinates of a pixel of the detector.
 *
 * The pixel is located at a distance of DOD from the volumetric center of the object.
 *
 * @param projection The projection data.
 * @param row The row index of the pixel.
 * @param col The column index of the pixel.
 * @return The 3D coordinates of the pixel.
 */
point3D getPixelPosition(const projection* projection, const int row, const int col);

/**
 * @brief Finds which axis is parallel to the ray.
 *
 * If the ray is not parallel to any axis, it returns axis::NONE.
 *
 * @param ray The ray to check.
 * @return The axis that is parallel to the ray, or axis::NONE if none.
 */
axis getParallelAxis(const ray ray);

/**
 * @brief Calculates the position of the plane parallel to the given axis and at the given index.
 *
 * @note Siddon's algorithm, equation 3
 *
 * \f$
 * X_{plane}(i) = X_{plane}(0) + i * d_x \quad (i = 0, \ldots, N_x) \\
 * Y_{plane}(j) = Y_{plane}(0) + j * d_y \quad (j = 0, \ldots, N_y) \\
 * Z_{plane}(k) = Z_{plane}(0) + k * d_z \quad (k = 0, \ldots, N_z)
 * \f$
 *
 * @param axis The axis parallel to the plane.
 * @param index The index of the plane.
 * @return The position of the plane.
 */
double getPlanePosition(const axis axis, const int index);

/**
 * @brief Calculates the intersection points of the ray with the planes.
 *
 * @note Siddon's algorithm, equation 4
 *
 * For each axis, the first element is the entry point of the ray into the first plane of that axis,
 * and the second element is the exit point of the ray from the last plane of that axis.
 *
 * \f$
 * \text{If}(X_2-X_1) \neq 0: \\
 * \quad\alpha_x(0) = (X_{plane}(0) - X_1) / (X_2 - X_1) \\
 * \quad\alpha_x(N_x) = (X_{plane}(N_x) - X_1) / (X_2 - X_1)
 * \f$
 *
 * If the ray is parallel to the axis, the intersection points are undefined.
 *
 * Similar equations are used for the Y and Z axes.
 *
 * @param ray The ray to calculate intersections for.
 * @param parallelTo The axis parallel to the ray.
 * @param intersections The array to store the intersection points.
 */
void getSidesIntersections(const ray ray, const axis parallelTo, double intersections[3][2]);

/**
 * @brief Finds the minimum intersection point along the given axis.
 *
 * @note Siddon's algorithm, equation 5
 *
 * \f$
 * \alpha_{min} = \max\{0, \min\{\alpha_x(0), \alpha_x(N_x)\}, \min\{\alpha_y(0), \alpha_y(N_y)\}, \min\{\alpha_z(0), \alpha_z(N_z)\}\}
 * \f$
 *
 * @param parallelTo The axis parallel to the ray.
 * @param intersections The array of intersection points.
 * @return The minimum intersection point.
 */
double getAMin(const axis parallelTo, double intersections[3][2]);

/**
 * @brief Finds the maximum intersection point along the given axis.
 *
 * @note Siddon's algorithm, equation 5
 *
 * \f$
 * \alpha_{max} = \min\{1, \max\{\alpha_x(0), \alpha_x(N_x)\}, \max\{\alpha_y(0), \alpha_y(N_y)\}, \max\{\alpha_z(0), \alpha_z(N_z)\}\}
 * \f$
 *
 * Similar equations are used for the Y and Z axes.
 *
 * @param parallelTo The axis parallel to the ray.
 * @param intersections The array of intersection points.
 * @return The maximum intersection point.
 */
double getAMax(const axis parallelTo, double intersections[3][2]);

/**
 * @brief Calculates the ranges of planes that the ray intersects.
 *
 * @note Siddon's algorithm, equation 6
 *
 * \f$
 * \text{If}(X_2-X_1) \geq 0: \\
 * \quad i_{min} = N_x - (X_{plane}(N_x) - \alpha_{min} * (X_2 - X_1) - X_1) / d_x \\
 * \quad i_{max} = 1 + (X_1 + \alpha_{max} * (X_2 - X_1) - X_{plane}(0)) / d_x \\
 * \text{If}(X_2-X_1) \leq 0: \\
 * \quad i_{min} = N_x - (X_{plane}(N_x) - \alpha_{max} * (X_2 - X_1) - X_1) / d_x \\
 * \quad i_{max} = 1 + (X_1 + \alpha_{min} * (X_2 - X_1) - X_{plane}(0)) / d_x
 * \f$
 *
 * Similar equations are used for j and k indexes.
 *
 * @param ray The ray to calculate ranges for.
 * @param planesIndexes The array to store the ranges of planes.
 * @param aMin The minimum intersection point.
 * @param aMax The maximum intersection point.
 */
void getPlanesRanges(const ray ray, range planesIndexes[3], const double aMin, const double aMax);

/**
 * @brief Calculates *all* the intersection points of the ray with the planes.
 *
 * @note Siddon's algorithm, equation 7
 *
 * \f$
 * \text{If}(X_2-X_1) \gt 0: \\
 * \quad \{\alpha_x\} = \{\alpha_x(i_{min}), \ldots, \alpha_x(i_{max})\} \\
 * \text{If}(X_2-X_1) \lt 0: \\
 * \quad \{\alpha_x\} = \{\alpha_x(i_{max}), \ldots, \alpha_x(i_{min})\} \\
 *
 * \text{where:} \\
 * \alpha_x(i) = (X_{plane}(i) - X_1) / (X_2 - X_1) \\
 *
 * \text{or recursively:} \\
 * \alpha_x(i) = \alpha_x(i-1) + d_x / (X_2 - X_1) \\
 * \f$
 *
 * Similar equations are used for the Y and Z axes.
 *
 * For each axis, the array contains the intersection points of the ray with the planes of that axis.
 *
 * @param ray The ray to calculate intersections for.
 * @param planesRanges The ranges of planes that the ray intersects.
 * @param a The array to store the intersection points.
 */
void getAllIntersections(const ray ray, const range planesRanges[3], double* a[3]);

/**
 * @brief Merges the intersection points of the ray with the planes.
 *
 * @note Siddon's algorithm, equation 8
 *
 * \f$
 * \{\alpha\} = \{\alpha_{min}, \text{merge}(\alpha_x, \alpha_y, \alpha_z), \alpha_{max}\} \\
 * \qquad = \{\alpha(0), \ldots, \alpha(N)\} \\
 * \text{where:} \\
 * N = (i_{max} - i_{min} + 1) + (j_{max} - j_{min} + 1) + (k_{max} - k_{min} + 1)
 * \f$
 *
 * The merged array contains the intersection points of the ray with all the planes, sorted in ascending order.
 *
 * @param aX The intersection points of the ray with the planes of the x-axis.
 * @param aY The intersection points of the ray with the planes of the y-axis.
 * @param aZ The intersection points of the ray with the planes of the z-axis.
 * @param aXSize The size of the aX array.
 * @param aYSize The size of the aY array.
 * @param aZSize The size of the aZ array.
 * @param aMerged The array to store the merged intersection points.
 */
void mergeIntersections(const double aX[], const double aY[], const double aZ[], const int aXSize, const int aYSize, const int aZSize, double aMerged[]);


#ifdef _DEBUG
/**
 * @brief Checks if the array is sorted in ascending order.
 *
 * @note this function is used for debugging purposes only.
 *
 * @param array The array to check.
 * @param size The size of the array.
 * @return true if the array is sorted, false otherwise.
 */
bool isArraySorted(const double array[], int size);
#endif

/**
 * @brief Computes the absorption value of the voxels intersected by the ray.
 *
 * @note Siddon's algorithm, equation 10
 *
 * \f$
 * l(m) = d_{12} * (\alpha(m) - \alpha(m-1)) \quad (m = 1, \ldots, N)
 * \f$
 *
 * @note Siddon's algorithm, equation 11
 *
 * \f$
 * d_{12} = \sqrt{(X_2 - X_1)^2 + (Y_2 - Y_1)^2 + (Z_2 - Z_1)^2}
 * \f$
 *
 * @note Siddon's algorithm, equation 12
 *
 * \f$
 * i(m) = 1 + (X_1 + \alpha_{mid} * (X_2 - X_1) - X_{plane}(0)) / d_x \\
 * j(m) = 1 + (Y_1 + \alpha_{mid} * (Y_2 - Y_1) - Y_{plane}(0)) / d_y \\
 * k(m) = 1 + (Z_1 + \alpha_{mid} * (Z_2 - Z_1) - Z_{plane}(0)) / d_z
 * \f$
 *
 * @note Siddon's algorithm, equation 13
 *
 * \f$
 * \alpha_{mid} = (\alpha(m) + \alpha(m-1)) / 2
 * \f$
 *
 * The absorption value is computed using the value that is assumed by the pixels
 * and the length of the intersection of the ray with the voxels.
 *
 * @param ray The ray to compute the absorption for.
 * @param a The absorption coefficients array.
 * @param lenA The size of the absorption coefficients array.
 * @param volume The volume structure containing the absorption coefficients.
 * @param projection The projection data.
 * @param pixelIndex The index of the pixel to get the absorption value for.
 */
void computeAbsorption(const ray ray, const double a[], const int lenA, const volume* volume, const projection* projection, const int pixelIndex);

/**
 * @brief Computes the backprojection of the projection.
 *
 * @note Siddon's algorithm, equation 9
 *
 * \f$
 * N = (i_{max} - i_{min} + 1) + (j_{max} - j_{min} + 1) + (k_{max} - k_{min} + 1)
 * \f$
 *
 * @note Siddon's algorithm, equation 8
 *
 * \f$
 * \{\alpha\} = \{\alpha_{min}, \text{merge}(\alpha_x, \alpha_y, \alpha_z), \alpha_{max}\} \\
 * \qquad = \{\alpha(0), \ldots, \alpha(N)\}
 * \f$
 *
 * The backprojection is computed by iterating over all the rays and computing
 * the absorption of the voxels intersected by the ray.
 *
 * @param projection The projection containing the projection pixels values.
 * @param volume The volume structure containing the absorption coefficients.
 */
void computeBackProjection(const projection* projection, volume* volume);
