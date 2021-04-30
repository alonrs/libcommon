#ifndef _REFCNT_H
#define _REFCNT_H

#include <stdint.h>
#include <stdatomic.h>

struct refcnt {
    atomic_uint val;
};

static inline void refcnt_init(struct refcnt *refcnt);
static inline void refcnt_destroy(struct refcnt *refcnt);
static inline void refcnt_ref(struct refcnt *refcnt);
/* Returns value of "refcnt" before change */
static inline uint32_t refcnt_unref(struct refcnt *refcnt);
/* Rarely used */
static inline void refcnt_set(struct refcnt *refcnt, uint32_t val);


static inline void
refcnt_init(struct refcnt *refcnt)
{
    atomic_init(&refcnt->val, 1);
}

static inline void
refcnt_destroy(struct refcnt *refcnt)
{
    ASSERT(refcnt->val == 0);
}

static inline void
refcnt_ref(struct refcnt *refcnt)
{
    atomic_fetch_add(&refcnt->val, 1);
}

static inline uint32_t
refcnt_unref(struct refcnt *refcnt)
{
    uint32_t val;
    val = atomic_fetch_sub(&refcnt->val, 1);
    ASSERT(val != 0xFFFFFF);
    return val;
}

static inline void
refcnt_set(struct refcnt *refcnt, uint32_t val)
{
    atomic_store(&refcnt->val, val);
}

#endif
