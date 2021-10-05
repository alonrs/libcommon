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
#include <math.h>

#include "random.h"
#include "hash.h"
#include "util.h"

static uint32_t seed = 0;

/* Win32 implementation of drand48 and srand48 */
#ifdef _WIN32
#define RAND48_SEED_0 (0x330e)
#define RAND48_SEED_1 (0xabcd)
#define RAND48_SEED_2 (0x1234)
#define RAND48_MULT_0 (0xe66d)
#define RAND48_MULT_1 (0xdeec)
#define RAND48_MULT_2 (0x0005)
#define RAND48_ADD    (0x000b)

uint16_t _rand48_seed[3] = {
    RAND48_SEED_0,
    RAND48_SEED_1,
    RAND48_SEED_2
};

uint16_t _rand48_mult[3] = {
    RAND48_MULT_0,
    RAND48_MULT_1,
    RAND48_MULT_2
};


uint16_t _rand48_add = RAND48_ADD;

static void
dorand48(uint16_t xseed[3])
{
    uint64_t accu;
    uint16_t temp[2];
    accu = (uint64_t)_rand48_mult[0] * (uint64_t)xseed[0] +
           (uint64_t)_rand48_add;
    temp[0] = (uint16_t)accu; /* lower 16 bits */
    accu >>= sizeof(uint16_t)* 8;
    accu += (uint64_t)_rand48_mult[0] * (uint64_t)xseed[1] +
            (uint64_t)_rand48_mult[1] * (uint64_t)xseed[0];
    temp[1] = (uint16_t)accu; /* middle 16 bits */
    accu >>= sizeof(uint16_t)* 8;
    accu += _rand48_mult[0] * xseed[2] +
            _rand48_mult[1] * xseed[1] +
            _rand48_mult[2] * xseed[0];
    xseed[0] = temp[0];
    xseed[1] = temp[1];
    xseed[2] = (uint16_t)accu;
}

static double
erand48(uint16_t xseed[3])
{
    dorand48(xseed);
    return ldexp((double) xseed[0], -48) +
           ldexp((double) xseed[1], -32) +
           ldexp((double) xseed[2], -16);
}

static double
drand48()
{
    return erand48(_rand48_seed);
}

static void
srand48(long seed)
{
    _rand48_seed[0] = RAND48_SEED_0;
    _rand48_seed[1] = (uint16_t)seed;
    _rand48_seed[2] = (uint16_t)(seed >> 16);
    _rand48_mult[0] = RAND48_MULT_0;
    _rand48_mult[1] = RAND48_MULT_1;
    _rand48_mult[2] = RAND48_MULT_2;
    _rand48_add = RAND48_ADD;
}

#endif /* _WIN32 */

static uint32_t random_next(void);

void
random_init(void)
{
    while (!seed) {
        uint32_t t = (uint32_t)time(NULL);
        seed = t;
        srand48(seed);
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
    while (!seed_) {
        seed_ = (uint32_t)time(NULL);
    }
    seed = seed_;
    srand48(seed_);
}

uint32_t
random_get_seed()
{
    return seed;
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
