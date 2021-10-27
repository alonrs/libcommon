#ifndef PRINT_UTILS_H
#define PRINT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

struct print_utils;

struct print_utils * print_utils_init(FILE *file);
void print_utils_destroy(struct print_utils *p);

void print_utils_reset(struct print_utils *p);
void print_utils_delete_last(struct print_utils *p);
void print_utils_printf(struct print_utils *p, const char *fmt, ...);
void print_utils_flush(struct print_utils *p);

#ifdef __cplusplus
}
#endif


#endif
