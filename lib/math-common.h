#ifndef _MATH_COMMON_H
#define _MATH_COMMON_H

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include "random.h"

int compare_floats(const void *a, const void *b);
int compare_integers(const void *a, const void *b);
double normal_distribution(double mu, double sigma);


#endif
