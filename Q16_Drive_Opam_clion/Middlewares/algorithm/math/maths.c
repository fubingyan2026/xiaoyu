/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <math.h>

#include "axis.h"
#include "maths.h"

#if defined(FAST_MATH) || defined(VERY_FAST_MATH)
#if defined(VERY_FAST_MATH)

// http://lolengine.net/blog/2011/12/21/better-function-approximations
// Chebyshev http://stackoverflow.com/questions/345085/how-do-trigonometric-functions-work/345117#345117
// Thanks for ledvinap for making such accuracy possible! See: https://github.com/cleanflight/cleanflight/issues/940#issuecomment-110323384
// https://github.com/Crashpilot1000/HarakiriWebstore1/blob/master/src/mw.c#L1235
// sin_approx maximum absolute error = 2.305023e-06
// cos_approx maximum absolute error = 2.857298e-06
#define sinPolyCoef3 (-1.666568107e-1f)
#define sinPolyCoef5  8.312366210e-3f
#define sinPolyCoef7 (-1.849218155e-4f)
#define sinPolyCoef9  0
#else
#define sinPolyCoef3 -1.666665710e-1f                                          // Double: -1.666665709650470145824129400050267289858e-1
#define sinPolyCoef5  8.333017292e-3f                                          // Double:  8.333017291562218127986291618761571373087e-3
#define sinPolyCoef7 -1.980661520e-4f                                          // Double: -1.980661520135080504411629636078917643846e-4
#define sinPolyCoef9  2.600054768e-6f                                          // Double:  2.600054767890361277123254766503271638682e-6
#endif

/**
 * @brief Computes an approximation of the sine of a given angle.
 *
 * This function calculates the sine of the input angle using a polynomial
 * approximation. The input angle is first wrapped to the range -π to π, and then
 * further reduced to the range -π/2 to π/2 to improve the accuracy of the
 * approximation. The polynomial coefficients are used to compute the sine value.
 *
 * @param x The input angle in radians for which to compute the sine.
 * @return The approximate value of the sine of the input angle.
 */
float sin_approx(float x)
{
    while (x > M_PIf) x -= 2.0f * M_PIf; // always wrap input angle to -PI..PI
    while (x < -M_PIf) x += 2.0f * M_PIf;
    if (x > 0.5f * M_PIf)
        x = 0.5f * M_PIf - (x - 0.5f * M_PIf); // We just pick -90..+90 Degree
    else if (x < -(0.5f * M_PIf))
        x = -(0.5f * M_PIf) - ((0.5f * M_PIf) + x);
    const float x2 = x * x;
    return x + x * x2 * (sinPolyCoef3 + x2 * (sinPolyCoef5 + x2 * (sinPolyCoef7 + x2 * sinPolyCoef9)));
}

/**
 * @brief Computes an approximation of the cosine of a given angle.
 *
 * This function calculates the cosine of the input angle by utilizing the
 * sine approximation function with a phase shift. The phase shift is applied
 * to convert the sine function into a cosine function, based on the trigonometric
 * identity: cos(x) = sin(x + π/2).
 *
 * @param x The input angle in radians for which to compute the cosine.
 * @return The approximate value of the cosine of the input angle.
 */
float cos_approx(const float x)
{
    return sin_approx(x + 0.5f * M_PIf);
}

// Initial implementation by Crashpilot1000 (https://github.com/Crashpilot1000/HarakiriWebstore1/blob/396715f73c6fcf859e0db0f34e12fe44bace6483/src/mw.c#L1292)
// Polynomial coefficients by Andor (http://www.dsprelated.com/showthread/comp.dsp/21872-1.php) optimized by Ledvinap to save one multiplication
// Max absolute error 0,000027 degree
// atan2_approx maximum absolute error = 7.152557e-07 rads (4.098114e-05 degree)
float atan2_approx(const float y, const float x)
{
#define atanPolyCoef1  3.14551665884836e-07f
#define atanPolyCoef2  0.99997356613987f
#define atanPolyCoef3  0.14744007058297684f
#define atanPolyCoef4  0.3099814292351353f
#define atanPolyCoef5  0.05030176425872175f
#define atanPolyCoef6  0.1471039133652469f
#define atanPolyCoef7  0.6444640676891548f

    float res;
    float absX = ABS_M(x);
    float absY = ABS_M(y);
    res = MAX_M(absX, absY);
    if (res) res = MIN_M(absX, absY) / res;
    else res = 0.0f;
    res = -((((atanPolyCoef5 * res - atanPolyCoef4) * res - atanPolyCoef3) * res - atanPolyCoef2) * res -
            atanPolyCoef1) / ((atanPolyCoef7 * res + atanPolyCoef6) * res + 1.0f);
    if (absY > absX) res = (M_PIf *0.5f) - res;
    if (x < 0) res = M_PIf - res;
    if (y < 0) res = -res;
    return res;
}

// http://http.developer.nvidia.com/Cg/acos.html
// Handbook of Mathematical Functions
// M. Abramowitz and I.A. Stegun, Ed.
// acos_approx maximum absolute error = 6.760856e-05 rads (3.873685e-03 degree)
float acos_approx(const float x)
{
    const float xa = fabsf(x);
    const float result = sqrtf(1.0f - xa) * (1.5707288f + xa * (-0.2121144f + xa * (0.0742610f + (-0.0187293f * xa))));
    if (x < 0.0f)
        return M_PIf - result;
    return result;
}

/**
 * @brief Computes an approximation of the arcsine (inverse sine) of a given value.
 *
 * This function calculates the arcsine of the input value by utilizing the
 * relationship between arcsine and arccosine. The formula used is
 * asin(x) = π/2 - acos(x), where `acos_approx` is a custom approximation for
 * the arccosine function.
 *
 * @param x The input value for which to compute the arcsine. Expected to be in the range [-1, 1].
 * @return The approximate value of the arcsine of the input value, in radians.
 */
float asin_approx(const float x)
{
    return M_PIf * 0.5f - acos_approx(x);
}

#endif

/**
 * @brief Computes the greatest common divisor (GCD) of two integers.
 *
 * This function uses the Euclidean algorithm to find the GCD of the two input
 * integers. The algorithm is based on the principle that the GCD of two numbers
 * also divides their difference. The process is repeated recursively until the
 * remainder is zero, at which point the GCD is the non-zero remainder.
 *
 * @param num The first integer.
 * @param denom The second integer.
 * @return The greatest common divisor of the two input integers.
 */
int gcd(const int num, const int denom)
{
    if (denom == 0) {
        return num;
    }

    return gcd(denom, num % denom);
}

/**
 * @brief Raises a given base to the power of an exponent.
 *
 * This function calculates the result of raising the `base` to the power
 * of `exp`. It iteratively multiplies the base by itself `exp - 1` times to
 * compute the final result. The function is designed for non-negative integer
 * exponents.
 *
 * @param base The base number to be raised to a power.
 * @param exp The exponent to which the base is raised. Must be a non-negative integer.
 * @return The result of base raised to the power of exp.
 */
float powerf(const float base, const int exp)
{
    float result = base;
    for (int count = 1; count < exp; count++) result *= base;

    return result;
}

/**
 * @brief Applies a deadband to the given value.
 *
 * This function checks if the absolute value of the input is less than the
 * specified deadband. If it is, the function returns 0, effectively applying
 * a deadband. If the value is outside the deadband, the function adjusts the
 * value by subtracting or adding the deadband, depending on whether the value
 * is positive or negative, respectively.
 *
 * @param value The input value to which the deadband will be applied.
 * @param deadband The size of the deadband. Values within this range (positive
 * and negative) will be set to 0.
 * @return The adjusted value after applying the deadband, or 0 if the value
 * was within the deadband.
 */
int32_t applyDeadband(const int32_t value, const int32_t deadband)
{
    if (ABS_M(value) < deadband) {
        return 0;
    }

    return value >= 0 ? value - deadband : value + deadband;
}

float fapplyDeadband(const float value, const float deadband)
{
    if (fabsf(value) < deadband) {
        return 0;
    }

    return value >= 0 ? value - deadband : value + deadband;
}

void devClear(stdev_t *dev)
{
    dev->m_n = 0;
}

/**
 * @brief Updates the running statistics with a new data point for standard deviation calculation.
 *
 * This function takes a pointer to a `stdev_t` structure and a new data point. It updates the
 * internal state of the structure to include the new data point, allowing for the incremental
 * computation of the mean and the sum of squared differences from the mean, which are used in
 * the calculation of the standard deviation.
 *
 * @param dev Pointer to the `stdev_t` structure that holds the running statistics.
 * @param x The new data point to be included in the statistics.
 */
void devPush(stdev_t *dev, const float x)
{
    dev->m_n++;
    if (dev->m_n == 1) {
        dev->m_oldM = dev->m_newM = x;
        dev->m_oldS = 0.0f;
    } else {
        dev->m_newM = dev->m_oldM + (x - dev->m_oldM) / dev->m_n;
        dev->m_newS = dev->m_oldS + (x - dev->m_oldM) * (x - dev->m_newM);
        dev->m_oldM = dev->m_newM;
        dev->m_oldS = dev->m_newS;
    }
}

float devVariance(const stdev_t *dev)
{
    return dev->m_n > 1 ? dev->m_newS / (dev->m_n - 1) : 0.0f;
}

float devStandardDeviation(const stdev_t *dev)
{
    return sqrtf(devVariance(dev));
}

float degreesToRadians(const int16_t degrees)
{
    return degrees * RAD;
}

int scaleRange(const int x, const int srcFrom, const int srcTo, const int destFrom, const int destTo)
{
    const long int a = ((long int) destTo - (long int) destFrom) * ((long int) x - (long int) srcFrom);
    const long int b = (long int) srcTo - (long int) srcFrom;
    return a / b + destFrom;
}

float scaleRangef(float x, float srcFrom, float srcTo, float destFrom, float destTo)
{
    float a = (destTo - destFrom) * (x - srcFrom);
    float b = srcTo - srcFrom;
    return (a / b) + destFrom;
}

// Normalize a vector
void normalizeV(const struct fp_vector *src, struct fp_vector *dest)
{
    const float length = sqrtf(src->X * src->X + src->Y * src->Y + src->Z * src->Z);
    if (length != 0) {
        dest->X = src->X / length;
        dest->Y = src->Y / length;
        dest->Z = src->Z / length;
    }
}

/**
 * @brief Constructs a 3x3 rotation matrix from the given angles.
 *
 * This function takes a set of Euler angles (roll, pitch, yaw) and constructs
 * a 3x3 rotation matrix. The rotation matrix is used to rotate vectors in 3D space.
 * The angles are first converted to their respective sine and cosine values using
 * polynomial approximations, and then these values are used to fill the rotation matrix.
 *
 * @param delta A pointer to a structure containing the roll, pitch, and yaw angles.
 * @param matrix A 3x3 matrix that will be filled with the rotation matrix elements.
 */
void buildRotationMatrix(const fp_angles_t *delta, float matrix[3][3])
{
    const float cosx = cos_approx(delta->angles.roll);
    const float sinx = sin_approx(delta->angles.roll);
    const float cosy = cos_approx(delta->angles.pitch);
    const float siny = sin_approx(delta->angles.pitch);
    const float cosz = cos_approx(delta->angles.yaw);
    const float sinz = sin_approx(delta->angles.yaw);

    const float coszcosx = cosz * cosx;
    const float sinzcosx = sinz * cosx;
    const float coszsinx = sinx * cosz;
    const float sinzsinx = sinx * sinz;

    matrix[0][X] = cosz * cosy;
    matrix[0][Y] = -cosy * sinz;
    matrix[0][Z] = siny;
    matrix[1][X] = sinzcosx + coszsinx * siny;
    matrix[1][Y] = coszcosx - sinzsinx * siny;
    matrix[1][Z] = -sinx * cosy;
    matrix[2][X] = sinzsinx - coszcosx * siny;
    matrix[2][Y] = coszsinx + sinzcosx * siny;
    matrix[2][Z] = cosy * cosx;
}

// Rotate a vector *v by the euler angles defined by the 3-vector *delta.
/**
 * @brief Rotates a vector according to the specified angles.
 *
 * This function applies a rotation to the given 3D vector `v` based on the
 * rotation angles provided in `delta`. The rotation is performed using a
 * rotation matrix, which is constructed from the roll, pitch, and yaw angles
 * contained in `delta`.
 *
 * @param v Pointer to the vector to be rotated. The vector will be modified to
 *          reflect the new orientation after rotation.
 * @param delta Pointer to a structure containing the rotation angles (roll,
 *              pitch, and yaw) used to build the rotation matrix.
 */
void rotateV(struct fp_vector *v, const fp_angles_t *delta)
{
    const struct fp_vector v_tmp = *v;

    float matrix[3][3];

    buildRotationMatrix(delta, matrix);

    v->X = v_tmp.X * matrix[0][X] + v_tmp.Y * matrix[1][X] + v_tmp.Z * matrix[2][X];
    v->Y = v_tmp.X * matrix[0][Y] + v_tmp.Y * matrix[1][Y] + v_tmp.Z * matrix[2][Y];
    v->Z = v_tmp.X * matrix[0][Z] + v_tmp.Y * matrix[1][Z] + v_tmp.Z * matrix[2][Z];
}

// Quick median filter implementation
// (c) N. Devillard - 1998
// http://ndevilla.free.fr/median/median.pdf
#define QMF_SORT(a, b) { if ((a)>(b)) QMF_SWAP((a),(b)); }
#define QMF_SWAP(a, b) { int32_t temp=(a);(a)=(b);(b)=temp; }
#define QMF_COPY(p, v, n) { int32_t i; for (i=0; i<n; i++) p[i]=v[i]; }
#define QMF_SORTF(a, b) { if ((a)>(b)) QMF_SWAPF((a),(b)); }
#define QMF_SWAPF(a, b) { float temp=(a);(a)=(b);(b)=temp; }

int32_t quickMedianFilter3(int32_t *v)
{
    int32_t p[3];
    QMF_COPY(p, v, 3);

    QMF_SORT(p[0], p[1]);
    QMF_SORT(p[1], p[2]);
    QMF_SORT(p[0], p[1]);
    return p[1];
}

int32_t quickMedianFilter5(int32_t *v)
{
    int32_t p[5];
    QMF_COPY(p, v, 5);

    QMF_SORT(p[0], p[1]);
    QMF_SORT(p[3], p[4]);
    QMF_SORT(p[0], p[3]);
    QMF_SORT(p[1], p[4]);
    QMF_SORT(p[1], p[2]);
    QMF_SORT(p[2], p[3]);
    QMF_SORT(p[1], p[2]);
    return p[2];
}

int32_t quickMedianFilter7(int32_t *v)
{
    int32_t p[7];
    QMF_COPY(p, v, 7);

    QMF_SORT(p[0], p[5]);
    QMF_SORT(p[0], p[3]);
    QMF_SORT(p[1], p[6]);
    QMF_SORT(p[2], p[4]);
    QMF_SORT(p[0], p[1]);
    QMF_SORT(p[3], p[5]);
    QMF_SORT(p[2], p[6]);
    QMF_SORT(p[2], p[3]);
    QMF_SORT(p[3], p[6]);
    QMF_SORT(p[4], p[5]);
    QMF_SORT(p[1], p[4]);
    QMF_SORT(p[1], p[3]);
    QMF_SORT(p[3], p[4]);
    return p[3];
}

int32_t quickMedianFilter9(int32_t *v)
{
    int32_t p[9];
    QMF_COPY(p, v, 9);

    QMF_SORT(p[1], p[2]);
    QMF_SORT(p[4], p[5]);
    QMF_SORT(p[7], p[8]);
    QMF_SORT(p[0], p[1]);
    QMF_SORT(p[3], p[4]);
    QMF_SORT(p[6], p[7]);
    QMF_SORT(p[1], p[2]);
    QMF_SORT(p[4], p[5]);
    QMF_SORT(p[7], p[8]);
    QMF_SORT(p[0], p[3]);
    QMF_SORT(p[5], p[8]);
    QMF_SORT(p[4], p[7]);
    QMF_SORT(p[3], p[6]);
    QMF_SORT(p[1], p[4]);
    QMF_SORT(p[2], p[5]);
    QMF_SORT(p[4], p[7]);
    QMF_SORT(p[4], p[2]);
    QMF_SORT(p[6], p[4]);
    QMF_SORT(p[4], p[2]);
    return p[4];
}

float quickMedianFilter3f(float *v)
{
    float p[3];
    QMF_COPY(p, v, 3);

    QMF_SORTF(p[0], p[1]);
    QMF_SORTF(p[1], p[2]);
    QMF_SORTF(p[0], p[1]);
    return p[1];
}

float quickMedianFilter5f(float *v)
{
    float p[5];
    QMF_COPY(p, v, 5);

    QMF_SORTF(p[0], p[1]);
    QMF_SORTF(p[3], p[4]);
    QMF_SORTF(p[0], p[3]);
    QMF_SORTF(p[1], p[4]);
    QMF_SORTF(p[1], p[2]);
    QMF_SORTF(p[2], p[3]);
    QMF_SORTF(p[1], p[2]);
    return p[2];
}

float quickMedianFilter7f(float *v)
{
    float p[7];
    QMF_COPY(p, v, 7);

    QMF_SORTF(p[0], p[5]);
    QMF_SORTF(p[0], p[3]);
    QMF_SORTF(p[1], p[6]);
    QMF_SORTF(p[2], p[4]);
    QMF_SORTF(p[0], p[1]);
    QMF_SORTF(p[3], p[5]);
    QMF_SORTF(p[2], p[6]);
    QMF_SORTF(p[2], p[3]);
    QMF_SORTF(p[3], p[6]);
    QMF_SORTF(p[4], p[5]);
    QMF_SORTF(p[1], p[4]);
    QMF_SORTF(p[1], p[3]);
    QMF_SORTF(p[3], p[4]);
    return p[3];
}

float quickMedianFilter9f(float *v)
{
    float p[9];
    QMF_COPY(p, v, 9);

    QMF_SORTF(p[1], p[2]);
    QMF_SORTF(p[4], p[5]);
    QMF_SORTF(p[7], p[8]);
    QMF_SORTF(p[0], p[1]);
    QMF_SORTF(p[3], p[4]);
    QMF_SORTF(p[6], p[7]);
    QMF_SORTF(p[1], p[2]);
    QMF_SORTF(p[4], p[5]);
    QMF_SORTF(p[7], p[8]);
    QMF_SORTF(p[0], p[3]);
    QMF_SORTF(p[5], p[8]);
    QMF_SORTF(p[4], p[7]);
    QMF_SORTF(p[3], p[6]);
    QMF_SORTF(p[1], p[4]);
    QMF_SORTF(p[2], p[5]);
    QMF_SORTF(p[4], p[7]);
    QMF_SORTF(p[4], p[2]);
    QMF_SORTF(p[6], p[4]);
    QMF_SORTF(p[4], p[2]);
    return p[4];
}

void arraySubInt32(int32_t *dest, const int32_t *array1, const int32_t *array2, const int count)
{
    for (int i = 0; i < count; i++) {
        dest[i] = array1[i] - array2[i];
    }
}

/**
 * @brief Converts a fixed-point number to a percentage.
 *
 * This function takes a 12-bit fixed-point number and converts it to a
 * percentage. The result is an integer value representing the percentage.
 *
 * @param q The fixed-point number to be converted to a percentage.
 * @return The percentage value as an int16_t.
 */
int16_t qPercent(const fix12_t q)
{
    return (100 * q) >> 12;
}

int16_t qMultiply(const fix12_t q, const int16_t input)
{
    return (input * q) >> 12;
}

fix12_t qConstruct(const int16_t num, const int16_t den)
{
    return (num << 12) / den;
}
