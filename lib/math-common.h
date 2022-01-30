#ifndef _MATH_COMMON_H
#define _MATH_COMMON_H

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include "random.h"

#ifdef __cplusplus
extern "C" {
#endif

int compare_floats(const void *a, const void *b);
int compare_integers(const void *a, const void *b);
int compare_uint64(const void *a, const void *b);
int compare_uint32(const void *a, const void *b);
double normal_distribution(double mu, double sigma);
double manual_ln(double input);
double manual_sqrt(double input);


#ifdef __cplusplus
}
#endif

#endif
