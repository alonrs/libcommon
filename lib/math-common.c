#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include "math-common.h"
#include "random.h"
#include "simd.h"

int
compare_floats(const void *a, const void *b)
{
    return *(float*)a >= *(float*)b;
}

int
compare_integers(const void *a, const void *b)
{
    return *(int*)a >= *(int*)b;
}

int
compare_uint32(const void *a, const void *b)
{
    return *(uint32_t*)a >= *(uint32_t*)b;
}

int
compare_uint64(const void *a, const void *b)
{
    return *(uint64_t*)a >= *(uint64_t*)b;
}

double
manual_sqrt(double input)
{
    PS_REG val;
    float f[SIMD_WIDTH] CACHE_ALIGNED;
    val = SIMD_SET1_PS((float)input);
    SIMD_SQRT_PS(val, val);
    SIMD_STORE_PS(f, val);
    return (double)f[0];
}

/* Works well for inputs >= 1/1024 */
double
manual_ln(double y)
{
    /* From: https://stackoverflow.com/a/44232045/4103200 */
    float divisor, x, result;
    int log2;
    const float ln2 = 0.69314718;
    const float ln_scaling_factor = 6.9314718;
    const float scaling_factor = 1024;

    if (!y) return NAN;

    y = y * scaling_factor;
    BITSCAN_REVERSE_UINT32(log2, y);
    divisor = (float)(1 << log2);
    x = y / divisor; /* normalized value between [1.0, 2.0] */
    result = (0.44717955 - 0.056570851 * x);
    result = (-1.4699568 + result * x);
    result = (2.8212026  + result * x);
    result = (-1.7417939 + result * x);
    result += ((float)log2) * ln2 - ln_scaling_factor;
    return result;
}

/* Randomizes numbers from the normal distribution N~(mu, sigma).
 * See: https://en.wikipedia.org/wiki/Marsaglia_polar_method
 */
double
normal_distribution(double mu, double sigma)
{
    double u1, u2, w, mult;
    static double x1, x2;
    static int call = 0;

    if (call == 1) {
        call = !call;
        return (mu + sigma * x2);
    }

    do {
        u1 = -1 + random_double()*2;
        u2 = -1 + random_double()*2;
        w = u1*u1 + u2*u2;
    } while (w >= 1 || w == 0);

    mult = manual_sqrt((-2 * manual_ln(w)) / w);
    x1 = u1 * mult;
    x2 = u2 * mult;
    call = !call;

    return (mu + sigma * x1);
}
