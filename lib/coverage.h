#ifndef COVERAGE_H
#define COVERAGE_H

#include <stdio.h>
#include "perf.h"

#ifdef COVERAGE
#define COVERAGE_MEASURE(NAME) PERF_START(__coverage_##NAME)
#else
#define COVERAGE_MEASURE(NAME)
#endif

#ifdef COVERAGE
#define COVERAGE_INC(NAME) \
    PERF_END(__coverage_##NAME);  \
    coverage_collect(__FILE__, __func__, #NAME, __coverage_##NAME);
#else
#define COVERAGE_INC(NAME)
#endif

#ifdef COVERAGE
#define COVERAGE_PRINT coverage_print(stdout);
#else
#define COVERAGE_PRINT
#endif

#ifdef __cplusplus
extern "C" {
#endif

void coverage_collect(const char *file,
                      const char *function,
                      const char *name,
                      double time);
void coverage_print(FILE *dst);
double coverage_get_avg_time(const char *name);
long coverage_get_counter(const char *name);

#ifdef __cplusplus
}
#endif

#endif
