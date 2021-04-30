/*
 * Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013 Nicira, Inc.
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
 */

#include <stdlib.h>
#include <time.h>

#include "random.h"
#include "hash.h"
#include "util.h"

static uint32_t seed = 0;

static uint32_t random_next(void);

void
random_init(void)
{
    while (!seed) {
        uint32_t t = (uint32_t)time(NULL);
        seed = t;
    }
}

/* Returns a random double in [0,1) */
double
random_double(void)
{
    return drand48();
}

void
random_set_seed(uint32_t seed_)
{
    seed = seed_;
}

void
random_bytes(void *p_, size_t n)
{
    uint8_t *p = p_;

    random_init();

    for (; n > 4; p += 4, n -= 4) {
        uint32_t x = random_next();
        memcpy(p, &x, 4);
    }

    if (n) {
        uint32_t x = random_next();
        memcpy(p, &x, n);
    }
}


uint32_t
random_uint32(void)
{
    random_init();
    return random_next();
}

uint64_t
random_uint64(void)
{
    uint64_t x;

    random_init();

    x = random_next();
    x |= (uint64_t) random_next() << 32;
    return x;
}

static uint32_t
random_next(void)
{
    uint32_t *seedp = &seed;

    *seedp ^= *seedp << 13;
    *seedp ^= *seedp >> 17;
    *seedp ^= *seedp << 5;

    return *seedp;
}
