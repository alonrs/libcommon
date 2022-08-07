#ifndef _LOCKS_H
#define _LOCKS_H

#ifndef __cplusplus
#include <stdatomic.h>
#else
#include <atomic>
#endif

#include <stdbool.h>
#include <pthread.h>
#include "util.h"

/* ===== Spinlock  ===== */

struct spinlock {
    const char *where;
#ifndef __cplusplus
    atomic_uint value;
#else
    std::atomic<uint32_t> value;
#endif
};

static inline void spinlock_init(struct spinlock *spin);
static inline void spinlock_destroy(struct spinlock *spin);

/* Locks "spin" and returns status code */
#define spinlock_lock(spin) spinlock_lock_at(spin, SOURCE_LOCATOR)
#define spinlock_try_lock(spin) spinlock_try_lock_at(spin, SOURCE_LOCATOR)
#define spinlock_wait(spin) spinlock_wait_at(spin, SOURCE_LOCATOR)

static inline void spinlock_wait_at(struct spinlock *spin, const char *where);
static inline void spinlock_lock_at(struct spinlock *spin, const char *where);
static inline void spinlock_unlock(struct spinlock *spin);
static inline bool spinlock_is_locked(struct spinlock *spin);

/* ===== Mutex  ===== */

struct mutex {
    const char *where;
    pthread_mutex_t lock;
};

static inline void mutex_init(struct mutex *mutex);
static inline void mutex_destroy(struct mutex *mutex);

#define mutex_lock(mutex) mutex_lock_at(mutex, SOURCE_LOCATOR);

static inline void mutex_lock_at(struct mutex *mutex_, const char *where);
static inline void mutex_unlock(struct mutex *mutex_);

/* ===== Condition variables  ===== */

struct cond {
    pthread_cond_t cond;
    struct mutex mutex;
#ifndef __cplusplus
    atomic_uint value;
#else
    std::atomic<uint32_t> value;
#endif
};

#define cond_wait(cond) cond_wait_at(cond, SOURCE_LOCATOR);
#define cond_lock(cond) cond_lock_at(cond, SOURCE_LOCATOR);

static inline void cond_init(struct cond *cond);
static inline void cond_destroy(struct cond *cond);
static inline void cond_wait_at(struct cond *cond, const char *where);
static inline void cond_lock_at(struct cond *cond, const char *where);
static inline void cond_unlock(struct cond *cond);
static inline bool cond_is_locked(struct cond *cond);

static inline void
spinlock_init(struct spinlock *spin)
{
    if (!spin) {
        return;
    }
#ifndef __cplusplus
    atomic_init(&spin->value, 0);
#else
    spin->value.store(0, std::memory_order_release);
#endif
    spin->where = NULL;
}

static inline void
spinlock_destroy(struct spinlock *spin)
{
    if (!spin) {
        return;
    }
#ifndef __cplusplus
    ASSERT(atomic_load(&spin->value)==0);
#else
    ASSERT(spin->value.load(std::memory_order_acquire)==0);
#endif
    spin->where = NULL;
}

static inline void
spinlock_lock_at(struct spinlock *spin, const char *where)
{
    if (!spin) {
        return;
    }
    uint32_t zero = 0;
#ifndef __cplusplus
    while (!atomic_compare_exchange_strong(&spin->value, &zero, 1)) {
        zero=0;
    }
#else
    while(!spin->value.compare_exchange_strong(zero, 1,
                                               std::memory_order_seq_cst)) {
        zero = 0;
    }
#endif
    spin->where = where;
}

/* Returns 1 iff the lock succeeded */
static inline int
spinlock_try_lock_at(struct spinlock *spin, const char *where)
{
    int result;
    if (!spin) {
        return 0;
    }
    uint32_t zero = 0;
#ifndef __cplusplus
    result = atomic_compare_exchange_strong(&spin->value, &zero, 1);
#else
    result =
       spin->value.compare_exchange_strong(zero, 1, std::memory_order_seq_cst);
#endif
    if (result) {
        spin->where = where;
    }
    return result;
}

static inline void
spinlock_wait_at(struct spinlock *spin, const char *where)
{
    if (!spin) {
        return;
    }
#ifndef __cplusplus
    while (atomic_load(&spin->value)==1);
#else
    while (spin->value.load(std::memory_order_acquire)==1);
#endif
    spin->where = where;
}

static inline void
spinlock_unlock(struct spinlock *spin)
{
    if (!spin) {
        return;
    }
#ifndef __cplusplus
    ASSERT(atomic_load(&spin->value)==1);
    atomic_store(&spin->value, 0);
#else
    ASSERT(spin->value.load(std::memory_order_acquire)==1);
    spin->value.store(0, std::memory_order_release);
#endif
    spin->where = NULL;
}

static inline bool
spinlock_is_locked(struct spinlock *spin)
{
#ifndef __cplusplus
    return atomic_load(&spin->value)==1;
#else
    return spin->value.load(std::memory_order_acquire) == 1;
#endif
}

static inline void
mutex_init(struct mutex *mutex)
{
    if (pthread_mutex_init(&mutex->lock, NULL)) {
        abort_msg("pthread_mutex_init fail");
    }
}

static inline void
mutex_destroy(struct mutex *mutex)
{
    if (pthread_mutex_destroy(&mutex->lock)) {
        abort_msg("pthread_mutex_destroy fail");
    }
}

static inline void
mutex_lock_at(struct mutex *mutex, const char *where)
{
    if (!mutex) {
        return;
    }
    if (pthread_mutex_lock(&mutex->lock)) {
        abort_msg("pthread_mutex_lock fail");
    }
    mutex->where = where;
}

static inline void
mutex_unlock(struct mutex *mutex)
{
    if (!mutex) {
        return;
    }
    if (pthread_mutex_unlock(&mutex->lock)) {
        abort_msg("pthread_mutex_unlock fail");
    }
    mutex->where = NULL;
}


static inline void
cond_init(struct cond *cond) {
    if (pthread_cond_init(&cond->cond, NULL)) {
        abort_msg("pthread_cond_init fail");
    }
    mutex_init(&cond->mutex);
    atomic_init(&cond->value, 0);
}

static inline void
cond_destroy(struct cond *cond) {
#ifndef __cplusplus
    uint32_t value = atomic_load(&cond->value);
#else
    uint32_t value = cond->value.load(std::memory_order_acquire);
#endif
    ASSERT(!value);
    if (pthread_cond_destroy(&cond->cond)) {
        abort_msg("pthread_cond_destroy fail");
    }
    mutex_destroy(&cond->mutex);
}

static inline void
cond_wait_at(struct cond *cond, const char *where) {
    uint32_t value;
    mutex_lock_at(&cond->mutex, where);
    while (1) {
#ifndef __cplusplus
        value = atomic_load(&cond->value);
#else
        value = cond->value.load(std::memory_order_acquire);
#endif
        if (!value) {
            break;
        }
        if (pthread_cond_wait(&cond->cond, &cond->mutex.lock)) {
            abort_msg("pthread_cond_wait fail");
        }
    }
    mutex_unlock(&cond->mutex);
}

static inline void
cond_lock_at(struct cond *cond, const char *where) {
#ifndef __cplusplus
    atomic_store(&cond->value, 1);
#else
    cond->value.store(1, std::memory_order_release);
#endif
}

static inline bool
cond_is_locked(struct cond *cond)
{
#ifndef __cplusplus
    uint32_t value = atomic_load(&cond->value);
#else
    uint32_t value = cond->value.load(std::memory_order_acquire);
#endif
    return value==1;
}

static inline void
cond_unlock(struct cond *cond) {
    mutex_lock(&cond->mutex);
#ifndef __cplusplus
    atomic_store(&cond->value, 0);
#else
    cond->value.store(0, std::memory_order_release);
#endif
    if (pthread_cond_broadcast(&cond->cond)) {
        abort_msg("pthread_cond_broadcast fail");
    }
    mutex_unlock(&cond->mutex);
}



#endif
