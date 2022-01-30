/*
 * Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2017 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modified by Igor De-Paula & Alon Rashelbach, Jan 2021.
 */

#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/* To make NO_RETURN portable across gcc/clang and MSVC, it should be
 * added at the beginning of the function declaration. */
#if __GNUC__ && !__CHECKER__
#define NO_RETURN __attribute__((__noreturn__))
#elif _MSC_VER
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN
#endif

#if __GNUC__ && !__CHECKER__
#define UNUSED __attribute__((__unused__))
#define PRINTF_FORMAT(FMT, ARG1) __attribute__((__format__(printf, FMT, ARG1)))
#define SCANF_FORMAT(FMT, ARG1) __attribute__((__format__(scanf, FMT, ARG1)))
#define WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
#define LIKELY(CONDITION) __builtin_expect(!!(CONDITION), 1)
#define UNLIKELY(CONDITION) __builtin_expect(!!(CONDITION), 0)
#else
#define UNUSED
#define PRINTF_FORMAT(FMT, ARG1)
#define SCANF_FORMAT(FMT, ARG1)
#define WARN_UNUSED_RESULT
#define LIKELY(CONDITION) (!!(CONDITION))
#define UNLIKELY(CONDITION) (!!(CONDITION))
#endif

/* Expands to a string that looks like "<file>:<line>", e.g. "tmp.c:10".
 *
 * See http://c-faq.com/ansi/stringize.html for an explanation of STRINGIZE
 * and STRINGIZE2. */
#define SOURCE_LOCATOR __FILE__ ":" STRINGIZE(__LINE__)
#define STRINGIZE(ARG) STRINGIZE2(ARG)
#define STRINGIZE2(ARG) #ARG

/* Bit masks */
#define BITMASK_8  0xff
#define BITMASK_16 0xffff
#define BITMASK_32 0xffffffff

/* This system's cache line size, in bytes.
 * Being wrong hurts performance but not correctness. */
#define CACHE_LINE_SIZE 64

/* Cacheline marking is typically done using zero-sized array. */
typedef uint8_t CACHE_LINE_MARKER[1];

/* This is a void expression that issues a compiler error if POINTER cannot be
 * compared for equality with the given pointer TYPE.  This generally means
 * that POINTER is a qualified or unqualified TYPE.  However,
 * BUILD_ASSERT_TYPE(POINTER, void *) will accept any pointer to object type,
 * because any pointer to object can be compared for equality with "void *".
 *
 * POINTER can be any expression.  The use of "sizeof" ensures that the
 * expression is not actually evaluated, so that any side effects of the
 * expression do not occur.
 *
 * The cast to int is present only to suppress an "expression using sizeof
 * bool" warning from "sparse" (see
 * http://permalink.gmane.org/gmane.comp.parsers.sparse/2967). */
#define BUILD_ASSERT_TYPE(POINTER, TYPE) \
    ((void) sizeof ((int) ((POINTER) == (TYPE) (POINTER))))

/* Casts 'pointer' to 'type' and issues a compiler warning if the cast changes
 * anything other than an outermost "const" or "volatile" qualifier. */
#define CONST_CAST(TYPE, POINTER)                               \
    (BUILD_ASSERT_TYPE(POINTER, TYPE),                          \
     (TYPE) (POINTER))

/* Given OBJECT of type pointer-to-structure, expands to the offset of MEMBER
 * within an instance of the structure.
 *
 * The GCC-specific version avoids the technicality of undefined behavior if
 * OBJECT is null, invalid, or not yet initialized.  This makes some static
 * checkers (like Coverity) happier.  But the non-GCC version does not actually
 * dereference any pointer, so it would be surprising for it to cause any
 * problems in practice.
 */
#ifdef __GNUC__
#define OBJECT_OFFSETOF(OBJECT, MEMBER) offsetof(typeof(*(OBJECT)), MEMBER)
#else
#define OBJECT_OFFSETOF(OBJECT, MEMBER) \
    ((char *) &(OBJECT)->MEMBER - (char *) (OBJECT))
#endif

/* Given a pointer-typed lvalue OBJECT, expands to a pointer type that may be
 * assigned to OBJECT. */
#ifdef __GNUC__
#define TYPEOF(OBJECT) typeof(OBJECT)
#else
#define TYPEOF(OBJECT) void *
#endif

/* Given POINTER, the address of the given MEMBER within an object of the type
 * that that OBJECT points to, returns OBJECT as an assignment-compatible
 * pointer type (either the correct pointer type or "void *").  OBJECT must be
 * an lvalue.
 *
 * This is the same as CONTAINER_OF except that it infers the structure type
 * from the type of '*OBJECT'. */
#define OBJECT_CONTAINING(POINTER, OBJECT, MEMBER)                      \
    ((TYPEOF(OBJECT)) (void *)                                      \
     ((char *) (POINTER) - OBJECT_OFFSETOF(OBJECT, MEMBER)))

/* Given POINTER, the address of the given MEMBER within an object of the type
 * that that OBJECT points to, assigns the address of the outer object to
 * OBJECT, which must be an lvalue.
 *
 * Evaluates to (void) 0 as the result is not to be used. */
#define ASSIGN_CONTAINER(OBJECT, POINTER, MEMBER) \
    ((OBJECT) = OBJECT_CONTAINING(POINTER, OBJECT, MEMBER), (void) 0)

/* As explained in the comment above OBJECT_OFFSETOF(), non-GNUC compilers
 * like MSVC will complain about un-initialized variables if OBJECT
 * hasn't already been initialized. To prevent such warnings, INIT_CONTAINER()
 * can be used as a wrapper around ASSIGN_CONTAINER. */
#define INIT_CONTAINER(OBJECT, POINTER, MEMBER) \
    ((OBJECT) = NULL, ASSIGN_CONTAINER(OBJECT, POINTER, MEMBER))

/* Yields the size of MEMBER within STRUCT. */
#define MEMBER_SIZEOF(STRUCT, MEMBER) (sizeof(((STRUCT *) NULL)->MEMBER))

/* Yields the offset of the end of MEMBER within STRUCT. */
#define OFFSETOFEND(STRUCT, MEMBER) \
        (offsetof(STRUCT, MEMBER) + MEMBER_SIZEOF(STRUCT, MEMBER))

/* Given POINTER, the address of the given MEMBER in a STRUCT object, returns
   the STRUCT object. */
#define CONTAINER_OF(POINTER, STRUCT, MEMBER)                           \
        ((STRUCT *) (void *) ((char *) (POINTER) - offsetof (STRUCT, MEMBER)))

/* Returns the address after the object pointed by POINTER casted to TYPE */
#define OBJECT_END(TYPE, POINTER) \
        (TYPE)((char*)POINTER+sizeof(*POINTER))

/* Returns the size in bytes of array with NUM elements of type TYPE */
#define ARRAY_SIZE(TYPE, NUM) \
        (sizeof(TYPE)*NUM)

/* Like the standard assert macro, except always evaluates the condition,
 * even with NDEBUG. */
#ifndef NDEBUG
#define ASSERT(CONDITION) \
    (LIKELY(CONDITION) ? (void) 0 : assert(0))
#else
#define ASSERT(CONDITION) ((void) (CONDITION))
#endif

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

/* Returns X / Y, rounding up.  X must be nonnegative to round correctly. */
#define DIV_ROUND_UP(X, Y) (((X) + ((Y) - 1)) / (Y))

/* Returns X rounded up to the nearest multiple of Y. */
#define ROUND_UP(X, Y) (DIV_ROUND_UP(X, Y) * (Y))

/* Returns the least number that, when added to X, yields a multiple of Y. */
#define PAD_SIZE(X, Y) (ROUND_UP(X, Y) - (X))

/* Returns X rounded down to the nearest multiple of Y. */
#define ROUND_DOWN(X, Y) ((X) / (Y) * (Y))

/* Returns true if X is a power of 2, otherwise false. */
#define IS_POW2(X) ((X) && !((X) & ((X) - 1)))

/* Align to cache line */
#define CACHE_ALIGNED __attribute__ ((aligned (64)))

/* Bitmap fast access */
#define ULLONG_SET0(MAP, OFFSET) ((MAP) &= ~(1ULL << (OFFSET)))
#define ULLONG_SET1(MAP, OFFSET) ((MAP) |= 1ULL << (OFFSET))
#define ULLONG_GET(MAP, OFFSET) !!((MAP) & (1ULL << (OFFSET)))

/* One cache line that acts as message accross cores. Access using NAME.val */
#define MESSAGE_T(TYPE, NAME) volatile static union CACHE_ALIGNED \
    { char _x[CACHE_LINE_SIZE]; TYPE val; } NAME

/* Expands to an anonymous union that contains:
 *
 *    - MEMBERS in a nested anonymous struct.
 *
 *    - An array as large as MEMBERS plus padding to a multiple of UNIT bytes.
 *
 * The effect is to pad MEMBERS to a multiple of UNIT bytes.
 *
 * For example, the struct below is 8 bytes long, with 6 bytes of padding:
 *
 *     struct padded_struct {
 *         PADDED_MEMBERS(8, uint8_t x; uint8_t y;);
 *     };
 */
#define PAD_PASTE2(x, y) x##y
#define PAD_PASTE(x, y) PAD_PASTE2(x, y)
#define PAD_ID PAD_PASTE(pad, __COUNTER__)
#ifndef __cplusplus
#define PADDED_MEMBERS(UNIT, MEMBERS)                               \
    union {                                                         \
        struct { MEMBERS };                                         \
        uint8_t PAD_ID[ROUND_UP(sizeof(struct { MEMBERS }), UNIT)]; \
    }
#else
/* C++ doesn't allow a type declaration within "sizeof", but it does support
 * scoping for member names, so we can just declare a second member, with a
 * name and the same type, and then use its size. */
#define PADDED_MEMBERS(UNIT, MEMBERS)                                       \
    struct named_member__ { MEMBERS };                                      \
    union {                                                                 \
        struct { MEMBERS };                                                 \
        uint8_t PAD_ID[ROUND_UP(sizeof(struct named_member__), UNIT)];      \
    }
#endif

/* Similar to PADDED_MEMBERS with additional cacheline marker:
 *
 *    - CACHE_LINE_MARKER is a cacheline marker
 *    - MEMBERS in a nested anonymous struct.
 *    - An array as large as MEMBERS plus padding to a multiple of UNIT bytes.
 *
 * The effect is to add cacheline marker and pad MEMBERS to a multiple of
 * UNIT bytes.
 *
 * Example:
 *     struct padded_struct {
 *         PADDED_MEMBERS_CACHELINE_MARKER(CACHE_LINE_SIZE, cacheline0,
 *             uint8_t x;
 *             uint8_t y;
 *         );
 *     };
 *
 * The PADDED_MEMBERS_CACHELINE_MARKER macro in above structure expands as:
 *
 *     struct padded_struct {
 *            union {
 *                    CACHE_LINE_MARKER cacheline0;
 *                    struct {
 *                            uint8_t x;
 *                            uint8_t y;
 *                    };
 *                    uint8_t         pad0[64];
 *            };
 *            *--- cacheline 1 boundary (64 bytes) ---*
 *     };
 */
#ifndef __cplusplus
#define PADDED_MEMBERS_CACHELINE_MARKER(UNIT, CACHELINE, MEMBERS)   \
    union {                                                         \
        CACHE_LINE_MARKER CACHELINE;                                \
        struct { MEMBERS };                                         \
        uint8_t PAD_ID[ROUND_UP(sizeof(struct { MEMBERS }), UNIT)]; \
    }
#else
#define PADDED_MEMBERS_CACHELINE_MARKER(UNIT, CACHELINE, MEMBERS)           \
    struct struct_##CACHELINE { MEMBERS };                                  \
    union {                                                                 \
        CACHE_LINE_MARKER CACHELINE;                                        \
        struct { MEMBERS };                                                 \
        uint8_t PAD_ID[ROUND_UP(sizeof(struct struct_##CACHELINE), UNIT)];  \
    }
#endif

/* ALIGNED_STRUCT may be used to define a structure whose instances should
 * aligned on an N-byte boundary.  This:
 *     ALIGNED_STRUCT(64, mystruct) { ... };
 * is equivalent to the following except that it specifies 64-byte alignment:
 *     struct mystruct { ... };
 *
 * ALIGNED_VAR defines a variable aligned on an N-byte boundary.  For
 * example,
 *     ALIGNED_VAR(64) int x;
 * defines a "int" variable that is aligned on a 64-byte boundary.
 */
#ifndef _MSC_VER
#define ALIGNED_STRUCT(N, TAG) struct __attribute__((aligned(N))) TAG
#define ALIGNED_VAR(N) __attribute__((aligned(N)))
#else
#define ALIGNED_STRUCT(N, TAG) __declspec(align(N)) struct TAG
#define ALIGNED_VAR(N) __declspec(align(N))
#endif


void abort_msg(const char *msg);

void *xmemdup(const void *, size_t);
void *xmalloc(size_t);
void *xmalloc_cacheline(size_t);
void *xzalloc_cacheline(size_t size);
void free_cacheline(void *);

uint32_t count_1bits(uint64_t x);

/* qsort auxiliary: compare integers from highest to lowest */
int int_compare_dec(void *a, void *b);

#ifdef __cplusplus
}
#endif

#endif
