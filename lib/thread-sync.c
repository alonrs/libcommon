#include <stdint.h>
#include <stdbool.h>
#include "thread-sync.h"

int
thread_sync_full_barrier(struct thread_sync *ts)
{
    uint64_t retval;
    uint64_t workers;
    uint64_t id;
    bool signal;

    signal = false;
    id = atomic_load(&ts->wait_id);

    while (1) {
        /* Check signal ID */
        if (atomic_load(&ts->wait_id) != id) {
            break;
        }
        if (!signal) {
            /* First time here */
            signal = true;
            retval = atomic_fetch_add(&ts->counter, 1);
        }
        workers = atomic_load(&ts->workers);
        /* I was the last thread to increase the counter! */
        if (retval >= workers-1) {
            /* Leader gets released, others wait for "thread_sync_continue" */
            return THREAD_SYNC_WAIT_LEADER;
        }
        usleep(30);
    }
    /* The signal has changed, return */
    return THREAD_SYNC_WAIT_WORKER;
}
