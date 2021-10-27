#include <stdlib.h>
#include <string.h>
#include "print-utils.h"
#include "util.h"

struct print_utils {
    size_t last_size;
    FILE *file;
};

struct print_utils *
print_utils_init(FILE *file)
{
    struct print_utils *out;
    out = xmalloc(sizeof(*out));
    out->last_size = 0;
    out->file = file;
    return out;
}

void
print_utils_destroy(struct print_utils *p)
{
    free(p);
}

void
print_utils_reset(struct print_utils *p)
{
    p->last_size = 0;
}

void
print_utils_delete_last(struct print_utils *p)
{
    for (size_t i=0; i<p->last_size; i++) {
        fprintf(p->file, "\b");
    }
    for (size_t i=0; i<p->last_size; i++) {
        fprintf(p->file, " ");
    }
    for (size_t i=0; i<p->last_size; i++) {
        fprintf(p->file, "\b");
    }
}

void
print_utils_printf(struct print_utils *p, const char *fmt, ...)
{
    int n;
    char *buffer;
    va_list args;

    va_start(args, fmt);

    n = vsnprintf(NULL, 0, fmt, args);
    buffer = xmalloc(sizeof(char)*(n+1));
    va_end(args);
    va_start(args, fmt);
    vsnprintf(buffer, n+1, fmt, args);
    fprintf(p->file, "%s", buffer);
    p->last_size += n;
    va_end(args);
    free(buffer);
}

void
print_utils_flush(struct print_utils *p)
{
    fflush(p->file);
}
