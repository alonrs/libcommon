#ifndef _THREAD_SYNC_H
#define _THREAD_SYNC_H

#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>

enum {
    THREAD_SYNC_WAIT_WORKER = 0,
    THREAD_SYNC_WAIT_LEADER = 1,
    THREAD_SYNC_WAIT_CHANGE = 2
};

/* Used to synchronize between threads, such that an operation that affects all
 * threads will happen exactly once. */

struct thread_sync {
    atomic_uint workers;
    atomic_uint counter;
    atomic_uint signal;
    atomic_ulong event_code;
    atomic_ulong event_args;
};

static inline void
thread_sync_init(struct thread_sync *ts)
{
    atomic_init(&ts->workers, 0);
    atomic_init(&ts->counter, 0);
    atomic_init(&ts->signal, 0);
    atomic_init(&ts->event_code, 0);
    atomic_init(&ts->event_args, 0);
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
static inline uint64_t
thread_sync_read_relaxed(struct thread_sync *ts)
{
    return ts->event_code;
}

/* Returns the even arguments */
static inline uint64_t
thread_sync_get_args(struct thread_sync *ts)
{
    return atomic_load(&ts->event_args);
}

/* Set a new event code */
static inline void
thread_sync_set_event(struct thread_sync *ts, uint64_t code, uint64_t args)
{
    uint32_t workers;

    workers = atomic_load(&ts->workers);
    atomic_store(&ts->event_code, code);
    atomic_store(&ts->event_args, args);
    atomic_store(&ts->counter, workers);
}

/* Waits until all threads receive the event with code "code", or until
 * the event code is changed. The last thread receives the message returns
 * "THREAD_SYNC_WAIT_LEADER", all others return "THREAD_SYNC_WAIT_WORKER".
 * If the event code is changed while waiting, returns
 * "THREAD_SYNC_WAIT_CHANGE" */
static int
thread_sync_wait(struct thread_sync *ts, uint64_t code)
{
    int retval;
    bool signal;

    signal = false;

    while (thread_sync_read_relaxed(ts) == code) {
        if (!signal) {
            retval = atomic_fetch_sub(&ts->counter, 1);
            signal = true;
        }
        if (retval == 1) {
            return THREAD_SYNC_WAIT_LEADER;
        }
        if (!atomic_load(&ts->counter)) {
            return THREAD_SYNC_WAIT_WORKER;
        }
        usleep(30);
    }
    return THREAD_SYNC_WAIT_CHANGE;
}

#endif
