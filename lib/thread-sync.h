#ifndef _THREAD_SYNC_H
#define _THREAD_SYNC_H

#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>
#include "locks.h"

enum {
    THREAD_SYNC_WAIT_WORKER = 0,
    THREAD_SYNC_WAIT_LEADER = 1,
};

/* Used to synchronize between threads, such that an operation that affects all
 * threads will happen exactly once. */

struct thread_sync {
    struct spinlock lock;
    atomic_uint workers;
    atomic_uint counter;
    atomic_uint signal;
    atomic_uint wait_id;
    atomic_ulong event_code;
    atomic_ulong event_args;
};

static inline void
thread_sync_init(struct thread_sync *ts)
{
    atomic_init(&ts->workers, 0);
    atomic_init(&ts->counter, 0);
    atomic_init(&ts->signal, 0);
    atomic_init(&ts->wait_id, 0);
    atomic_init(&ts->event_code, 0);
    atomic_init(&ts->event_args, 0);
    spinlock_init(&ts->lock);
}

/* Register a new worker thread */
static inline void
thread_sync_register(struct thread_sync *ts)
{
    atomic_fetch_add(&ts->workers, 1);
}

/* Unregister an existingworker thread */
static inline void
thread_sync_unregister(struct thread_sync *ts)
{
    atomic_fetch_sub(&ts->workers, 1);
}

/* Read the current event code, relaxed */
static inline void
thread_sync_read_relaxed(struct thread_sync *ts,
                         uint64_t *code,
                         uint64_t *args)
{
    if (code) {
        *code = ts->event_code;
    }
    if (args) {
        *args = ts->event_args;
    }
}

/* Read the current event code, relaxed */
static inline void
thread_sync_read_explicit(struct thread_sync *ts,
                          uint64_t *code,
                          uint64_t *args)
{
    if (code) {
        *code = atomic_load(&ts->event_code);
    }
    if (args) {
        *args = atomic_load(&ts->event_args);
    }
}

/* Set a new event code */
static inline void
thread_sync_set_event(struct thread_sync *ts, uint64_t code, uint64_t args)
{
    spinlock_lock(&ts->lock);
    atomic_store(&ts->event_code, code);
    atomic_store(&ts->event_args, args);
    spinlock_unlock(&ts->lock);
}

/* Releases all worker threads that are stuck within
 * "thread_sync_full_barrier" */
static inline void
thread_sync_continue(struct thread_sync *ts)
{
    spinlock_lock(&ts->lock);
    atomic_store(&ts->counter, 0);
    atomic_fetch_add(&ts->wait_id, 1);
    spinlock_unlock(&ts->lock);
};

/* Waits until all threads enter this method. The last thread receives the
 * message returns "THREAD_SYNC_WAIT_LEADER", and returns. All other threads
 * are kept in busy-wait until "thread_sync_continue" is invoked, then return 
 * "THREAD_SYNC_WAIT_WORKER". Note: the behavior of this is undefined if
 * the number of workers is changed while one or more of the threads are in
 * this */
int thread_sync_full_barrier(struct thread_sync *ts);


#endif
