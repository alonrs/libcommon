#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include "math-common.h"
#include "random.h"

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
        w = pow(u1, 2) + pow(u2, 2);
    } while (w >= 1 || w == 0);

    mult = sqrt((-2 * log (w)) / w);
    x1 = u1 * mult;
    x2 = u2 * mult;
    call = !call;

    return (mu + sigma * x1);
}
