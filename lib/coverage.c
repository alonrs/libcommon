#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "map.h"
#include "coverage.h"
#include "util.h"
#include "hash.h"

static int coverage_init_flag = 0;
static struct map map_coverage;

struct coverage_node {
    struct map_node *node;
    const char *name;
    uint64_t counter;
    double time;
};

static void
coverage_init()
{
    map_init(&map_coverage, 16);
    coverage_init_flag = 1;
}

static struct coverage_node *
coverage_node_new(const char *name)
{
    struct coverage_node *node;
    int hash;

    hash = hash_string(name, 0);
    node = xmalloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->name = strdup(name);
    map_insert(&map_coverage, node->node, hash);

    return node;
}

static struct coverage_node *
find_name(const char *name)
{
    struct coverage_node *node;
    int hash;

    hash = hash_string(name, 0);
    MAP_FOR_EACH_WITH_HASH(node, node, hash, &map_coverage) {
        if (!strcmp(name, node->name)) {
            return node;
        }
    }
    return NULL;
}

void
coverage_collect(const char *name, double time)
{
    struct coverage_node *node;

    if (!coverage_init_flag) {
        coverage_init();
    }

    node = find_name(name);
    if (!node) {
        node = coverage_node_new(name);
    }

    node->counter++;
    node->time += time;
}

double
coverage_get_avg_time(const char *name)
{
    struct coverage_node *node;
    node = find_name(name);
    if (!node) {
        return -1;
    }
    return node->time / node->counter;
}

long
coverage_get_counter(const char *name)
{
    struct coverage_node *node;
    node = find_name(name);
    if (!node) {
        return -1;
    }
    return node->counter;
}

void
coverage_print(FILE *dst)
{
    struct coverage_node *node;
    MAP_FOR_EACH(node, node, &map_coverage) {
        fprintf(dst, "%s: %lu hits, avg %.3f msec per hit\n",
                node->name, node->counter, node->time / node->counter / 1e6);
    }
}

