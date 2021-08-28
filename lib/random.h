/*
 * Copyright (c) 2008, 2009, 2010, 2012 Nicira, Inc.
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
 *
 * Modified by Alon Rashelbach, Jan 2021
 */

#ifndef _RANDOM_H
#define _RANDOM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void random_init(void);
void random_set_seed(uint32_t);
uint32_t random_get_seed();

void random_bytes(void *, size_t);
uint32_t random_uint32(void);
uint64_t random_uint64(void);

/* Returns a random double in [0,1) */
double random_double(void);

static inline int
random_range(int max)
{
    return random_uint32() % max;
}

static inline uint8_t
random_uint8(void)
{
    return random_uint32();
}

static inline uint16_t
random_uint16(void)
{
    return random_uint32();
}

/* Flips a coin, returns true with probability "prob" (in 0.0-1.0) */
static inline bool
random_coin(double prob)
{
    return random_double() <= prob;
}

#endif /* random.h */
