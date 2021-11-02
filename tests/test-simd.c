#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lib/random.h"
#include "lib/simd.h"

#define CHECK_NUM 5000000

static void
test_init(int argc, char **argv)
{
    int seed;

    if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
        printf("Usage: %s [seed] [vervosity] \n"
               "Use --help or -h to show this message.\n"
               "* seed: empty or 0 for random seed\n"
               "* verbosity: empty or 0 for low verbosity\n", argv[0]);
        exit(1);
    }

    seed = (argc <= 1) ? 0 : atoi(argv[1]);
    random_set_seed(seed);
    printf("Running with seed %u\n", random_get_seed());
    fflush(stdout);
}

static void
test_reduce_max()
{
    uint32_t epu32[SIMD_WIDTH];
    uint64_t epu64[SIMD_WIDTH64];
    uint32_t emax32;
    uint64_t emax64;
    int batch_size32;
    int batch_size64;
    uint32_t max32;
    uint64_t max64;

    emax32 = 0;
    emax64 = 0;
    max32 = 0;
    max64 = 0;
    batch_size32 = random_uint32() % SIMD_WIDTH;
    batch_size64 = random_uint32() % SIMD_WIDTH64;

    for (int i=0;i<SIMD_WIDTH;i++) {
        epu32[i] = random_uint32();
    }
    for (int i=0;i<SIMD_WIDTH64;i++) {
        epu64[i] = random_uint64();
    }

    for (int i=0;i<batch_size32;i++) {
        if (epu32[i] > emax32) emax32 = epu32[i];
    }
    for (int i=0;i<batch_size64;i++) {
        if (epu64[i] > emax64) emax64 = epu64[i];
    }

    EPU_REG v32 = SIMD_LOADU_SI(epu32);
    EPU_REG m32;
    EPU_REG64 v64 = SIMD_LOADU_SI(epu64);
    EPU_REG64 m64;

    SIMD_GENERATE_MASK_EPU32(m32, batch_size32);
    SIMD_GENERATE_MASK_EPU64(m64, batch_size64);

    v32 = SIMD_AND_SI(v32, m32);
    v64 = SIMD_AND_SI(v64, m64);

    SIMD_REDUCE_MAX_EPU32(max32, v32);
    SIMD_REDUCE_MAX_EPU64(max64, v64);

    if ((max32 != emax32) || (max64 != emax64)) {
        printf("\nError!\n");
        exit(EXIT_FAILURE);
    }
}

static void
test_reduce_sum()
{
    uint32_t epu32[SIMD_WIDTH];
    uint64_t epu64[SIMD_WIDTH64];
    uint32_t esum32;
    uint64_t esum64;
    int batch_size32;
    int batch_size64;
    int sum32;
    long sum64;

    esum32 = 0;
    esum64 = 0;
    sum32 = 0;
    sum64 = 0;
    batch_size32 = random_uint32() % SIMD_WIDTH;
    batch_size64 = random_uint32() % SIMD_WIDTH64;
    
    for (int i=0;i<SIMD_WIDTH;i++) {
        epu32[i] = random_uint32();
    }
    for (int i=0;i<SIMD_WIDTH64;i++) {
        epu64[i] = random_uint64();
    }

    for (int i=0;i<batch_size32;i++) {
        esum32 += epu32[i];
    }
    for (int i=0;i<batch_size64;i++) {
        esum64 += epu64[i];
    }

    EPU_REG v32 = SIMD_LOADU_SI(epu32);
    EPU_REG m32;
    EPU_REG64 v64 = SIMD_LOADU_SI(epu64);
    EPU_REG64 m64;

    SIMD_GENERATE_MASK_EPU32(m32, batch_size32);
    SIMD_GENERATE_MASK_EPU64(m64, batch_size64);

    v32 = SIMD_AND_SI(v32, m32);
    v64 = SIMD_AND_SI(v64, m64);

    SIMD_REDUCE_SUM_EPI32(sum32, v32);
    SIMD_REDUCE_SUM_EPI64(sum64, v64);

    if ((sum32 != esum32) || (sum64 != esum64)) {
        printf("\nError!\n");
        exit(EXIT_FAILURE);
    }
}

int
main(int argc, char **argv)
{
    test_init(argc, argv);

    printf("Performing tests");
    fflush(stdout);

    for (int count=0; count<CHECK_NUM; count++) {
        test_reduce_sum();
        test_reduce_max();
        if (!(count % (CHECK_NUM/10))) {
            printf(".");
            fflush(stdout);
        }
    }
    printf("\nDone\n");
}
