#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

#include "lib/util.h"
#include "lib/random.h"
#include "lib/hash.h"
#include "lib/cmap.h"
#include "lib/perf.h"

#define DEFAULT_SECONDS 3
#define DEFAULT_READERS 3

struct elem {
    struct map_node node;
    uint32_t value;
};

static size_t num_values;
static uint32_t max_value;
static uint32_t *values;
static uint32_t hash_base;
static struct map map_values;
static volatile bool running;
static volatile bool error;

static atomic_size_t checks;
static volatile uint32_t inserts;
static volatile uint32_t removes;

/* Insert new value to map */
static void
insert_value(uint32_t value)
{
    struct elem *elem;
    elem=(struct elem*)xmalloc(sizeof(*elem));
    elem->value = value;
    map_insert(&map_values, &elem->node, hash_int(value, hash_base));
}

/* Initiate all static variables */
static void
initiate_values(uint32_t seed)
{
    running = true;
    error = false;
    inserts = 0;
    removes = 0;
    random_set_seed(seed);
    num_values = (random_uint32() & 0xFF) + 16;
    max_value = (random_uint32() & 4096) + 2048;
    hash_base = random_uint32();
    values = (uint32_t*)xmalloc(sizeof(*values)*num_values);
    map_init(&map_values);
    atomic_init(&checks, 0);

    for (int i=0; i<num_values; i++) {
        values[i] = random_uint32() & max_value;
        insert_value(values[i]);
    }
}

/* Destroy all static variables */
static void
destroy_values()
{
    struct map_state map_state;
    struct elem *elem;
    free(values);

    map_state = map_state_acquire(&map_values);
    MAP_FOR_EACH(elem, node, map_state) {
        map_remove(&map_values, &elem->node);
        free(elem);
    }
    map_state_release(map_state);
    
    map_destroy(&map_values);
}


/* Returns true iff "value" can be composed from two integers in "map" */
static bool
can_compose_value(uint32_t value)
{
    struct elem *elem;
    uint32_t hash;
    struct map_state map_state = map_state_acquire(&map_values);

    hash = hash_int(value, hash_base);
    MAP_FOR_EACH_WITH_HASH(elem, node, hash, map_state) {
        if (elem->value == value) {
            map_state_release(map_state);
            return true;
        }
    }

    map_state_release(map_state);
    return false;
}

static inline void
wait()
{
    usleep(1);
}

/* Constantly writes and removes values from map */
static void*
update_map(void *args)
{
    struct elem *elem;
    uint32_t hash;
    struct map_state map_state;
    uint32_t value;

    while (running) {
        /* Insert */
        value = random_uint32() + max_value+1;
        insert_value(value);
        inserts++;
        wait();

        /* Remove */
        hash = hash_int(random_uint32(), hash_base);
        map_state = map_state_acquire(&map_values);
        MAP_FOR_EACH_WITH_HASH(elem, node, hash, map_state) {
            if (elem->value > max_value) {
                map_remove(&map_values, &elem->node);
                free(elem);
                removes++;
                break;
            }
        }
        map_state_release(map_state);
        wait();
    }
    return NULL;
}

/* Constantly check whever values in map can be composed */
static void*
read_map(void *args)
{
    uint32_t index;

    while (running) {
        index = random_uint32() % num_values;
        if(!can_compose_value(values[index])) {
            running = false;
            error = true;
            break;
        }
        atomic_fetch_add(&checks, 1);
        wait();
        if (can_compose_value(values[index]+max_value+1)) {
            running = false;
            error = true;
            break;
        }
        atomic_fetch_add(&checks, 1);
        wait();
    }
    return NULL;
}


int main(int argc, char **argv)
{
    /* Parse arguments */
    for (int i=0; i<argc; i++) {
        if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
            printf("Tests performance and correctness of cmap.\n"
                   "Usage: %s [SECONDS] [READERS]\n"
                   "Defaults: %d seconds, %d reader threads.\n",
                   argv[0], DEFAULT_SECONDS, DEFAULT_READERS);
            exit(1);
        }
    }
    int seconds = argc >=2 ? atoi(argv[1]) : DEFAULT_SECONDS;
    int readers = argc >=3 ? atoi(argv[2]) : DEFAULT_READERS;
    pthread_t *threads;

    /* Initiate */
    initiate_values(1); /* TODO - set seed */
    threads=(pthread_t*)xmalloc(sizeof(*threads)*(readers+1));

    /* Start threads */
    for (int i=0; i<readers; ++i) {
        pthread_create(&threads[i], NULL, read_map, NULL);
    }
    pthread_create(&threads[readers], NULL, update_map, NULL);

    /* Print stats to user */
    size_t dst = get_time_ns() + 1e9 * seconds;
    size_t current_checks;
    while (get_time_ns() < dst) {
        current_checks = atomic_load(&checks);
        printf("#checks: %ld, #inserts: %u, #removes: %u, "
               "map elements: %ld, utilization: %.2lf \n",
               current_checks, inserts, removes,
               map_size(&map_values),
               map_utilization(&map_values));
        usleep(250e3);
    }

    /* Stop threads */
    running = false;
    for (int i=0; i<=readers; ++i) {
        pthread_join(threads[i], NULL);
    }

    /* Delete memory */
    free(threads);
    destroy_values();

    /* Check for correctness errors */
    if (error) {
        printf("Error: correctness issue\n");
    }

    return error;
}
