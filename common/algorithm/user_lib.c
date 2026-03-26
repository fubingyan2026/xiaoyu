#include "user_lib.h"

/**
 * @brief Computes the inverse square root of a given floating-point number using an optimized algorithm.
 * @param num The input value for which the inverse square root is to be computed.
 * @retval The inverse square root of the input value.
 */
float invSqrt(const float num)
{
    const float halfnum = 0.5f * num;
    float y = num;
    long i = *(long *) &y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *) &i;
    y = y * (1.5f - halfnum * y * y);
    return y;
}

/**
 * @brief Computes an approximation of the square root of a given floating-point number using a fast algorithm.
 * @param num The input value for which the square root is to be computed.
 * @retval The approximate square root of the input value.
 */
float fastSqrt(const float num)
{
    return num * invSqrt(num);
}
