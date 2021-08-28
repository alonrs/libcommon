#ifndef COVERAGE_H
#define COVERAGE_H

#include <stdio.h>
#include "perf.h"

#ifdef COVERAGE
#define COVERAGE_MEASURE PERF_START(__coverage)
#else
#define COVERAGE_MEASURE
#endif

#ifdef COVERAGE
#define COVERAGE_INC(NAME) \
    PERF_END(__coverage);  \
    coverage_collect(__FILE__ ## ":" ## __func__ ## ":" NAME, __coverage);
#else
#define COVERAGE_INC(NAME)
#endif

void coverage_collect(const char *name, double time);
void coverage_print(FILE *dst);
double coverage_get_avg_time(const char *name);
long coverage_get_counter(const char *name);

#endif
