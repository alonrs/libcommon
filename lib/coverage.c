#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "coverage.h"
#include "util.h"
#include "hash.h"

static int coverage_init_flag = 0;
static struct map map_coverage;

struct coverage_node {
    struct map_node node;
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
    map_insert(&map_coverage, &node->node, hash);

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
coverage_collect(const char *file,
                 const char *function,
                 const char *name,
                 double time)
{
    struct coverage_node *node;
    size_t size;
    char *buff;

    if (!coverage_init_flag) {
        coverage_init();
    }

    size = strlen(file) + strlen(function) + strlen(name) + 3;
    buff = malloc(sizeof(char)*size);
    memset(buff, 0, size);
    strcat(buff, file);
    strcat(buff, ":");
    strcat(buff, function);
    strcat(buff, ":");
    strcat(buff, name);

    node = find_name(buff);
    if (!node) {
        node = coverage_node_new(buff);
    }

    node->counter++;
    node->time += time;
    free(buff);
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

    if (!coverage_init_flag) {
        coverage_init();
    }

    MAP_FOR_EACH(node, node, &map_coverage) {
        fprintf(dst, "%s %u hits, avg %.3f usec per hit\n",
                node->name, (uint32_t)node->counter,
                node->time / node->counter / 1e3);
    }
}

