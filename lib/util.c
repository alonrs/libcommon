#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"

int
int_compare_dec(void *a, void *b)
{
    int *int_a, *int_b;
    int_a=(int*)a;
    int_b=(int*)b;
    if (int_a > int_b) {
        return -1;
    } else if (int_a < int_b) {
        return 1;
    } else {
        return 0;
    }
}

/* unsigned int count_1bits(uint64_t x):
 *
 * Returns the number of 1-bits in 'x', between 0 and 64 inclusive. */
#if UINTPTR_MAX == UINT64_MAX
uint32_t
count_1bits(uint64_t x)
{
#if (__GNUC__ >= 4 && __POPCNT__) || (defined(__aarch64__) && __GNUC__ >= 7)
    return __builtin_popcountll(x);
#elif defined(__aarch64__) && __GNUC__ >= 6
    return vaddv_u8(vcnt_u8(vcreate_u8(x)));
#else
    /* This portable implementation is the fastest one we know of for 64
     * bits, and about 3x faster than GCC 4.7 __builtin_popcountll(). */
    const uint64_t h55 = UINT64_C(0x5555555555555555);
    const uint64_t h33 = UINT64_C(0x3333333333333333);
    const uint64_t h0F = UINT64_C(0x0F0F0F0F0F0F0F0F);
    const uint64_t h01 = UINT64_C(0x0101010101010101);
    x -= (x >> 1) & h55;               /* Count of each 2 bits in-place. */
    x = (x & h33) + ((x >> 2) & h33);  /* Count of each 4 bits in-place. */
    x = (x + (x >> 4)) & h0F;          /* Count of each 8 bits in-place. */
    return (x * h01) >> 56;            /* Sum of all bytes. */
#endif
}
#else /* Not 64-bit. */
#if __GNUC__ >= 4 && __POPCNT__
static inline unsigned int
count_1bits_32__(uint32_t x)
{
    return __builtin_popcount(x);
}
#else
#define NEED_COUNT_1BITS_8 1
extern const uint8_t count_1bits_8[256];
static inline unsigned int
count_1bits_32__(uint32_t x)
{
    /* This portable implementation is the fastest one we know of for 32 bits,
     * and faster than GCC __builtin_popcount(). */
    return (count_1bits_8[x & 0xff] +
            count_1bits_8[(x >> 8) & 0xff] +
            count_1bits_8[(x >> 16) & 0xff] +
            count_1bits_8[x >> 24]);
}
#endif
uint32_t
count_1bits(uint64_t x)
{
    return count_1bits_32__(x) + count_1bits_32__(x >> 32);
}
#endif


/* The C standards say that neither the 'dst' nor 'src' argument to
 * memcpy() may be null, even if 'n' is zero.  This wrapper tolerates
 * the null case. */
static inline void
nullable_memcpy(void *dst, const void *src, size_t n)
{
    if (n) {
        memcpy(dst, src, n);
    }
}

void
abort_msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    abort();
}

void *
xmalloc(size_t size)
{
    void *p = malloc(size ? size : 1);
    if (p == NULL) {
        abort_msg("Out of memory");
    }
    return p;
}

void *
xmemdup(const void *p_, size_t size)
{
    void *p = xmalloc(size);
    nullable_memcpy(p, p_, size);
    return p;
}

/* Allocates and returns 'size' bytes of memory aligned to 'alignment' bytes.
 * 'alignment' must be a power of two and a multiple of sizeof(void *).
 *
 * Use free_size_align() to free the returned memory block. */
void *
xmalloc_size_align(size_t size, size_t alignment)
{
#ifdef HAVE_POSIX_MEMALIGN
    void *p;
    int error;

    error = posix_memalign(&p, alignment, size ? size : 1);
    if (error != 0) {
        abort_msg("Out of memory");
    }
    return p;
#else
    /* Allocate room for:
     *
     *     - Header padding: Up to alignment - 1 bytes, to allow the
     *       pointer 'q' to be aligned exactly sizeof(void *) bytes before the
     *       beginning of the alignment.
     *
     *     - Pointer: A pointer to the start of the header padding, to allow us
     *       to free() the block later.
     *
     *     - User data: 'size' bytes.
     *
     *     - Trailer padding: Enough to bring the user data up to a alignment
     *       multiple.
     *
     * +---------------+---------+------------------------+---------+
     * | header        | pointer | user data              | trailer |
     * +---------------+---------+------------------------+---------+
     * ^               ^         ^
     * |               |         |
     * p               q         r
     *
     */
    void *p, *r, **q;
    bool runt;

    if (!IS_POW2(alignment) || (alignment % sizeof(void *) != 0)) {
        abort_msg("Invalid alignment");
    }

    p = xmalloc((alignment - 1)
                + sizeof(void *)
                + ROUND_UP(size, alignment));

    runt = PAD_SIZE((uintptr_t) p, alignment) < sizeof(void *);
    /* When the padding size < sizeof(void*), we don't have enough room for
     * pointer 'q'. As a reuslt, need to move 'r' to the next alignment.
     * So ROUND_UP when xmalloc above, and ROUND_UP again when calculate 'r'
     * below.
     */
    r = (void *) ROUND_UP((uintptr_t) p + (runt ? alignment : 0), alignment);
    q = (void **) r - 1;
    *q = p;

    return r;
#endif
}

/* Allocates and returns 'size' bytes of memory aligned to a cache line and in
 * dedicated cache lines.  That is, the memory block returned will not share a
 * cache line with other data, avoiding "false sharing".
 *
 * Use free_cacheline() to free the returned memory block. */
void *
xmalloc_cacheline(size_t size)
{
    return xmalloc_size_align(size, CACHE_LINE_SIZE);
}

/* Like xmalloc_cacheline() but clears the allocated memory to all zero
 * bytes. */
void *
xzalloc_cacheline(size_t size)
{
    void *p = xmalloc_cacheline(size);
    memset(p, 0, size);
    return p;
}

void
free_size_align(void *p)
{
#ifdef HAVE_POSIX_MEMALIGN
    free(p);
#else
    if (p) {
        void **q = (void **) p - 1;
        free(*q);
    }
#endif
}

/* Frees a memory block allocated with xmalloc_cacheline() or
 * xzalloc_cacheline(). */
void
free_cacheline(void *p)
{
    free_size_align(p);
}

