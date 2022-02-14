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
check_abort(int cond)
{
    if (cond) {
        return;
    }
    printf("\nError!\n");
    exit(EXIT_FAILURE);
}

static void
test_bitscan_reverse()
{
    uint32_t epu32 = random_uint32();
    uint64_t epu64 = random_uint64();
    int bsr32, bsr64;
    BITSCAN_REVERSE_UINT32(bsr32, epu32);
    BITSCAN_REVERSE_UINT64(bsr64, epu64);
    for (int i=0; i<32; i++) {
        if (!epu32) {
            check_abort(bsr32==i-1);
            break;
        }
        epu32>>=1;
    }
    for (int i=0; i<64; i++) {
        if (!epu64) {
            check_abort(bsr64==i-1);
            break;
        }
        epu64>>=1;
    }
}

static void
test_bitscan_forward()
{
    uint32_t epu32 = random_uint32();
    uint64_t epu64 = random_uint64();
    int bsf32, bsf64;
    BITSCAN_FORWARD_UINT32(bsf32, epu32);
    BITSCAN_FORWARD_UINT64(bsf64, epu64);
    for (int i=0; i<32; i++) {
        if (epu32&1) {
            check_abort(bsf32==i);
            break;
        }
        epu32>>=1;
    }
    for (int i=0; i<64; i++) {
        if (epu64&1) {
            check_abort(bsf64==i);
            break;
        }
        epu64>>=1;
    }
}

static void
test_move_mask_ps()
{
    uint32_t epu32[SIMD_WIDTH];
    EPU_REG r;
    PS_REG p;
    int result;
    for (int i=0; i<SIMD_WIDTH; i++) {
        epu32[i] = (random_uint32() & 0xF) < 7 ? 0 : 0xFFFFFFFF;
    }
    r = SIMD_LOADU_SI(epu32);
    p = SIMD_CASTSI_PS(r);
    SIMD_MOVE_MASK_PS(result,p);
    for (int i=0; i<SIMD_WIDTH; ++i) {
        if (epu32[i]) {
            check_abort((result & 1) == 1);
        } else {
            check_abort((result & 1) == 0);
        }
        result >>= 1;
    }
}

static void
test_move_mask_epi8()
{
    const int size = SIMD_WIDTH*sizeof(uint32_t);
    uint8_t epu8[size];
    EPU_REG r;
    int result;
    for (int i=0; i<size; i++) {
        epu8[i] = (random_uint32() & 0xF) < 7 ? 0 : 0xFF;
    }
    r = SIMD_LOADU_SI(epu8);
    SIMD_MOVE_MASK_EPI8(result,r);
    for (int i=0; i<size; i++) {
        if (epu8[i]) {
            check_abort((result & 1) == 1);
        } else {
            check_abort((result & 1) == 0);
        }
        result >>= 1;
    }
}

static void
test_load_store_epu()
{
    uint32_t epu32[SIMD_WIDTH];
    uint32_t target[SIMD_WIDTH] CACHE_ALIGNED;
    EPU_REG r;
    for (int i=0; i<SIMD_WIDTH; i++) {
        epu32[i] = random_uint32();
    }
    r = SIMD_LOADU_SI(epu32);
    SIMD_STORE_SI(target, r);
    for (int i=0; i<SIMD_WIDTH; i++) {
        check_abort(target[i]==epu32[i]);
    }
}

static void
test_load_store_ps()
{
    float ps32[SIMD_WIDTH];
    float target[SIMD_WIDTH] CACHE_ALIGNED;
    PS_REG r;
    for (int i=0; i<SIMD_WIDTH; i++) {
        ps32[i] = random_double();
    }
    r = SIMD_LOADU_PS(ps32);
    SIMD_STORE_PS(target, r);
    for (int i=0; i<SIMD_WIDTH; i++) {
        check_abort(target[i]==ps32[i]);
    }
}

static void
test_set1()
{
    uint32_t epu32[SIMD_WIDTH] CACHE_ALIGNED;
    float ps32[SIMD_WIDTH] CACHE_ALIGNED;
    uint64_t epu64[SIMD_WIDTH64] CACHE_ALIGNED;
    uint32_t rand_epu32;
    float rand_ps32;
    uint64_t rand_epu64;

    rand_epu32 = random_uint32();
    rand_epu64 = random_uint64();
    rand_ps32 = random_double();

    EPU_REG epu_reg;
    PS_REG ps_reg;
    EPU_REG64 epu64_reg;

    epu_reg = SIMD_SET1_EPI32(rand_epu32);
    ps_reg = SIMD_SET1_PS(rand_ps32);
    epu64_reg = SIMD_SET1_EPI64(rand_epu64);

    SIMD_STORE_SI(epu32, epu_reg);
    SIMD_STORE_PS(ps32, ps_reg);
    SIMD_STORE_SI(epu64, epu64_reg);

    for (int i=0; i<SIMD_WIDTH; i++) {
        check_abort(epu32[i] == rand_epu32);
        check_abort(ps32[i] == rand_ps32);
    }
    for (int i=0; i<SIMD_WIDTH64; i++) {
        check_abort(epu64[i] == rand_epu64);
    }
}

static void
test_set()
{
    /* 32bit */
    uint32_t a[8];
    for (int i=0; i<SIMD_WIDTH; i++) {
        a[i] = random_uint32();
    }
    EPU_REG epu_reg = SIMD_SET_EPI32(a[0],a[1],a[2],a[3],
                                     a[4],a[5],a[6],a[7]);
    uint32_t epu32[SIMD_WIDTH] CACHE_ALIGNED;
    SIMD_STORE_SI(epu32, epu_reg);
    for (int i=0; i<SIMD_WIDTH; i++) {
        check_abort(epu32[i] == a[SIMD_WIDTH-i-1]);
    }

    /* 64 bit */
    uint64_t a64[4];
    for (int i=0; i<SIMD_WIDTH64; i++) {
        a64[i] = random_uint64();
    }
    EPU_REG64 epu_reg64 = SIMD_SET_EPI64(a64[0],a64[1],a64[2],a64[3]);
    uint64_t epu64[SIMD_WIDTH64] CACHE_ALIGNED;
    SIMD_STORE_SI(epu64, epu_reg64);
    for (int i=0; i<SIMD_WIDTH64; i++) {
        check_abort(epu64[i] == a64[SIMD_WIDTH64-i-1]);
    }
}

static void
test_zeros_ffs()
{
    uint32_t epu32[SIMD_WIDTH] CACHE_ALIGNED;
    float ps32[SIMD_WIDTH] CACHE_ALIGNED;
    uint32_t *ptr;
    EPU_REG epu_reg;
    PS_REG ps_reg;
    /* Zeros */
    epu_reg = SIMD_ZEROS_SI;
    ps_reg = SIMD_ZEROS_PS;
    SIMD_STORE_SI(epu32, epu_reg);
    SIMD_STORE_PS(ps32, ps_reg);
    for (int i=0; i<SIMD_WIDTH; ++i) {
        check_abort(epu32[i] == 0);
        check_abort(ps32[i] == 0);
    }
    /* FFs */
    epu_reg = SIMD_FFS_SI;
    ps_reg = SIMD_FFS_PS;
    SIMD_STORE_SI(epu32, epu_reg);
    SIMD_STORE_PS(ps32, ps_reg);
    for (int i=0; i<SIMD_WIDTH; ++i) {
        check_abort(epu32[i] == 0xFFFFFFFF);
        ptr = (uint32_t*)&ps32[i];
        check_abort(*ptr == 0xFFFFFFFF);
    }
}

static void
test_add()
{
    uint32_t epu32[SIMD_WIDTH] CACHE_ALIGNED;
    uint64_t epu64[SIMD_WIDTH64] CACHE_ALIGNED;
    float ps32[SIMD_WIDTH] CACHE_ALIGNED;
    uint32_t epu32_1[SIMD_WIDTH] CACHE_ALIGNED;
    uint64_t epu64_1[SIMD_WIDTH64] CACHE_ALIGNED;
    float ps32_1[SIMD_WIDTH] CACHE_ALIGNED;
    EPU_REG epu_reg;
    EPU_REG64 epu_reg64;
    PS_REG ps_reg;

    for (int i=0; i<SIMD_WIDTH; ++i) {
        epu32[i] = random_uint32();
        ps32[i] = random_double();
    }
    for (int i=0; i<SIMD_WIDTH64; ++i) {
        epu64[i] = random_uint64();
    }

    epu_reg = SIMD_LOADU_SI(epu32);
    ps_reg = SIMD_LOADU_PS(ps32);
    epu_reg64 = SIMD_LOADU_SI64(epu64);

    epu_reg = SIMD_ADD_EPI32(epu_reg, epu_reg);
    epu_reg64 = SIMD_ADD_EPI64(epu_reg64, epu_reg64);
    ps_reg = SIMD_ADD_PS(ps_reg, ps_reg);

    SIMD_STORE_SI(epu32_1, epu_reg);
    SIMD_STORE_PS(ps32_1, ps_reg);
    SIMD_STORE_SI(epu64_1, epu_reg64);
    for (int i=0; i<SIMD_WIDTH; ++i) {
        check_abort(epu32_1[i] == 2* epu32[i]);
        check_abort(ps32_1[i] == 2* ps32[i]);
    }
    for (int i=0; i<SIMD_WIDTH64; ++i) {
        check_abort(epu64_1[i] == 2*epu64[i]);
    }
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

static void
test_alignr_epi8()
{
    const int size = SIMD_WIDTH*sizeof(uint32_t);
#ifndef NSIMD
    const int lane = 4*sizeof(uint32_t);
#else
    const int lane = sizeof(uint32_t);
#endif
    uint8_t epu8_a[size] CACHE_ALIGNED;
    uint8_t epu8_b[size] CACHE_ALIGNED;
    uint8_t epu8_c[size] CACHE_ALIGNED;
    int imm8;
    EPU_REG a, b, dst;

    for (int i=0; i<size; i++) {
        epu8_a[i] = (uint8_t)random_uint32();
        epu8_b[i] = (uint8_t)random_uint32();
    }

    imm8 = random_uint32() & (SIMD_WIDTH - 1);
    a = SIMD_LOADU_SI(epu8_a);
    b = SIMD_LOADU_SI(epu8_b);
    switch (imm8) {
    case 1: SIMD_ALIGNR_EPI8(dst, a, b, 1); break;
    case 2: SIMD_ALIGNR_EPI8(dst, a, b, 2); break;
    case 3: SIMD_ALIGNR_EPI8(dst, a, b, 3); break;
#ifndef NSIMD
    case 4: SIMD_ALIGNR_EPI8(dst, a, b, 4); break;
    case 5: SIMD_ALIGNR_EPI8(dst, a, b, 5); break;
    case 6: SIMD_ALIGNR_EPI8(dst, a, b, 6); break;
    case 7: SIMD_ALIGNR_EPI8(dst, a, b, 7); break;
    case 8: SIMD_ALIGNR_EPI8(dst, a, b, 8); break;
    case 9: SIMD_ALIGNR_EPI8(dst, a, b, 9); break;
    case 10: SIMD_ALIGNR_EPI8(dst, a, b, 10); break;
    case 11: SIMD_ALIGNR_EPI8(dst, a, b, 11); break;
    case 12: SIMD_ALIGNR_EPI8(dst, a, b, 12); break;
    case 13: SIMD_ALIGNR_EPI8(dst, a, b, 13); break;
    case 14: SIMD_ALIGNR_EPI8(dst, a, b, 14); break;
    case 15: SIMD_ALIGNR_EPI8(dst, a, b, 15); break;
    case 16: SIMD_ALIGNR_EPI8(dst, a, b, 16); break;
    case 17: SIMD_ALIGNR_EPI8(dst, a, b, 17); break;
    case 18: SIMD_ALIGNR_EPI8(dst, a, b, 18); break;
    case 19: SIMD_ALIGNR_EPI8(dst, a, b, 19); break;
    case 20: SIMD_ALIGNR_EPI8(dst, a, b, 20); break;
    case 21: SIMD_ALIGNR_EPI8(dst, a, b, 21); break;
    case 22: SIMD_ALIGNR_EPI8(dst, a, b, 22); break;
    case 23: SIMD_ALIGNR_EPI8(dst, a, b, 23); break;
    case 24: SIMD_ALIGNR_EPI8(dst, a, b, 24); break;
    case 25: SIMD_ALIGNR_EPI8(dst, a, b, 25); break;
    case 26: SIMD_ALIGNR_EPI8(dst, a, b, 26); break;
    case 27: SIMD_ALIGNR_EPI8(dst, a, b, 27); break;
    case 28: SIMD_ALIGNR_EPI8(dst, a, b, 28); break;
    case 29: SIMD_ALIGNR_EPI8(dst, a, b, 29); break;
    case 30: SIMD_ALIGNR_EPI8(dst, a, b, 30); break;
    case 31: SIMD_ALIGNR_EPI8(dst, a, b, 31); break;
    case 32: SIMD_ALIGNR_EPI8(dst, a, b, 32); break;
#endif
    default: SIMD_ALIGNR_EPI8(dst, a, b, 0); break;
    }

    SIMD_STORE_SI(epu8_c, dst);

    for (int i=0; i<lane - imm8; ++i) {
        check_abort(epu8_c[i] == epu8_b[i+imm8]);
        if (lane != size) {
            check_abort(epu8_c[i+lane] == epu8_b[i+imm8+lane]);
        }
    }
    for (int i=0; i<imm8; ++i) {
        check_abort(epu8_c[lane - imm8 + i] == epu8_a[i]);
        if (lane != size) {
            check_abort(epu8_c[2*lane - imm8 + i] == epu8_a[i+lane]);
        }
    }
}

static void
test_shuffle_epi32()
{
    const int size = SIMD_WIDTH;
#ifndef NSIMD
    const int lane = 4;
#else
    const int lane = 1;
#endif
    uint32_t epu8_a[size] CACHE_ALIGNED;
    uint32_t epu8_b[size] CACHE_ALIGNED;
    int imm8;
    EPU_REG a, dst;

    for (int i=0; i<size; i++) {
        epu8_a[i] = (uint32_t)random_uint32();
    }

    imm8 = (uint8_t)random_uint32();
    a = SIMD_LOADU_SI(epu8_a);

    switch (imm8) {
#ifndef NSIMD
    case 1: SIMD_SHUFFLE_EPI32(dst, a, 1); break;
    case 2: SIMD_SHUFFLE_EPI32(dst, a, 2); break;
    case 3: SIMD_SHUFFLE_EPI32(dst, a, 3); break;
    case 4: SIMD_SHUFFLE_EPI32(dst, a, 4); break;
    case 5: SIMD_SHUFFLE_EPI32(dst, a, 5); break;
    case 6: SIMD_SHUFFLE_EPI32(dst, a, 6); break;
    case 7: SIMD_SHUFFLE_EPI32(dst, a, 7); break;
    case 8: SIMD_SHUFFLE_EPI32(dst, a, 8); break;
    case 9: SIMD_SHUFFLE_EPI32(dst, a, 9); break;
    case 10: SIMD_SHUFFLE_EPI32(dst, a, 10); break;
    case 11: SIMD_SHUFFLE_EPI32(dst, a, 11); break;
    case 12: SIMD_SHUFFLE_EPI32(dst, a, 12); break;
    case 13: SIMD_SHUFFLE_EPI32(dst, a, 13); break;
    case 14: SIMD_SHUFFLE_EPI32(dst, a, 14); break;
    case 15: SIMD_SHUFFLE_EPI32(dst, a, 15); break;
    case 16: SIMD_SHUFFLE_EPI32(dst, a, 16); break;
    case 17: SIMD_SHUFFLE_EPI32(dst, a, 17); break;
    case 18: SIMD_SHUFFLE_EPI32(dst, a, 18); break;
    case 19: SIMD_SHUFFLE_EPI32(dst, a, 19); break;
    case 20: SIMD_SHUFFLE_EPI32(dst, a, 20); break;
    case 21: SIMD_SHUFFLE_EPI32(dst, a, 21); break;
    case 22: SIMD_SHUFFLE_EPI32(dst, a, 22); break;
    case 23: SIMD_SHUFFLE_EPI32(dst, a, 23); break;
    case 24: SIMD_SHUFFLE_EPI32(dst, a, 24); break;
    case 25: SIMD_SHUFFLE_EPI32(dst, a, 25); break;
    case 26: SIMD_SHUFFLE_EPI32(dst, a, 26); break;
    case 27: SIMD_SHUFFLE_EPI32(dst, a, 27); break;
    case 28: SIMD_SHUFFLE_EPI32(dst, a, 28); break;
    case 29: SIMD_SHUFFLE_EPI32(dst, a, 29); break;
    case 30: SIMD_SHUFFLE_EPI32(dst, a, 30); break;
    case 31: SIMD_SHUFFLE_EPI32(dst, a, 31); break;
    case 32: SIMD_SHUFFLE_EPI32(dst, a, 32); break;
    case 33: SIMD_SHUFFLE_EPI32(dst, a, 33); break;
    case 34: SIMD_SHUFFLE_EPI32(dst, a, 34); break;
    case 35: SIMD_SHUFFLE_EPI32(dst, a, 35); break;
    case 36: SIMD_SHUFFLE_EPI32(dst, a, 36); break;
    case 37: SIMD_SHUFFLE_EPI32(dst, a, 37); break;
    case 38: SIMD_SHUFFLE_EPI32(dst, a, 38); break;
    case 39: SIMD_SHUFFLE_EPI32(dst, a, 39); break;
    case 40: SIMD_SHUFFLE_EPI32(dst, a, 40); break;
    case 41: SIMD_SHUFFLE_EPI32(dst, a, 41); break;
    case 42: SIMD_SHUFFLE_EPI32(dst, a, 42); break;
    case 43: SIMD_SHUFFLE_EPI32(dst, a, 43); break;
    case 44: SIMD_SHUFFLE_EPI32(dst, a, 44); break;
    case 45: SIMD_SHUFFLE_EPI32(dst, a, 45); break;
    case 46: SIMD_SHUFFLE_EPI32(dst, a, 46); break;
    case 47: SIMD_SHUFFLE_EPI32(dst, a, 47); break;
    case 48: SIMD_SHUFFLE_EPI32(dst, a, 48); break;
    case 49: SIMD_SHUFFLE_EPI32(dst, a, 49); break;
    case 50: SIMD_SHUFFLE_EPI32(dst, a, 50); break;
    case 51: SIMD_SHUFFLE_EPI32(dst, a, 51); break;
    case 52: SIMD_SHUFFLE_EPI32(dst, a, 52); break;
    case 53: SIMD_SHUFFLE_EPI32(dst, a, 53); break;
    case 54: SIMD_SHUFFLE_EPI32(dst, a, 54); break;
    case 55: SIMD_SHUFFLE_EPI32(dst, a, 55); break;
    case 56: SIMD_SHUFFLE_EPI32(dst, a, 56); break;
    case 57: SIMD_SHUFFLE_EPI32(dst, a, 57); break;
    case 58: SIMD_SHUFFLE_EPI32(dst, a, 58); break;
    case 59: SIMD_SHUFFLE_EPI32(dst, a, 59); break;
    case 60: SIMD_SHUFFLE_EPI32(dst, a, 60); break;
    case 61: SIMD_SHUFFLE_EPI32(dst, a, 61); break;
    case 62: SIMD_SHUFFLE_EPI32(dst, a, 62); break;
    case 63: SIMD_SHUFFLE_EPI32(dst, a, 63); break;
    case 64: SIMD_SHUFFLE_EPI32(dst, a, 64); break;
    case 65: SIMD_SHUFFLE_EPI32(dst, a, 65); break;
    case 66: SIMD_SHUFFLE_EPI32(dst, a, 66); break;
    case 67: SIMD_SHUFFLE_EPI32(dst, a, 67); break;
    case 68: SIMD_SHUFFLE_EPI32(dst, a, 68); break;
    case 69: SIMD_SHUFFLE_EPI32(dst, a, 69); break;
    case 70: SIMD_SHUFFLE_EPI32(dst, a, 70); break;
    case 71: SIMD_SHUFFLE_EPI32(dst, a, 71); break;
    case 72: SIMD_SHUFFLE_EPI32(dst, a, 72); break;
    case 73: SIMD_SHUFFLE_EPI32(dst, a, 73); break;
    case 74: SIMD_SHUFFLE_EPI32(dst, a, 74); break;
    case 75: SIMD_SHUFFLE_EPI32(dst, a, 75); break;
    case 76: SIMD_SHUFFLE_EPI32(dst, a, 76); break;
    case 77: SIMD_SHUFFLE_EPI32(dst, a, 77); break;
    case 78: SIMD_SHUFFLE_EPI32(dst, a, 78); break;
    case 79: SIMD_SHUFFLE_EPI32(dst, a, 79); break;
    case 80: SIMD_SHUFFLE_EPI32(dst, a, 80); break;
    case 81: SIMD_SHUFFLE_EPI32(dst, a, 81); break;
    case 82: SIMD_SHUFFLE_EPI32(dst, a, 82); break;
    case 83: SIMD_SHUFFLE_EPI32(dst, a, 83); break;
    case 84: SIMD_SHUFFLE_EPI32(dst, a, 84); break;
    case 85: SIMD_SHUFFLE_EPI32(dst, a, 85); break;
    case 86: SIMD_SHUFFLE_EPI32(dst, a, 86); break;
    case 87: SIMD_SHUFFLE_EPI32(dst, a, 87); break;
    case 88: SIMD_SHUFFLE_EPI32(dst, a, 88); break;
    case 89: SIMD_SHUFFLE_EPI32(dst, a, 89); break;
    case 90: SIMD_SHUFFLE_EPI32(dst, a, 90); break;
    case 91: SIMD_SHUFFLE_EPI32(dst, a, 91); break;
    case 92: SIMD_SHUFFLE_EPI32(dst, a, 92); break;
    case 93: SIMD_SHUFFLE_EPI32(dst, a, 93); break;
    case 94: SIMD_SHUFFLE_EPI32(dst, a, 94); break;
    case 95: SIMD_SHUFFLE_EPI32(dst, a, 95); break;
    case 96: SIMD_SHUFFLE_EPI32(dst, a, 96); break;
    case 97: SIMD_SHUFFLE_EPI32(dst, a, 97); break;
    case 98: SIMD_SHUFFLE_EPI32(dst, a, 98); break;
    case 99: SIMD_SHUFFLE_EPI32(dst, a, 99); break;
    case 100: SIMD_SHUFFLE_EPI32(dst, a, 100); break;
    case 101: SIMD_SHUFFLE_EPI32(dst, a, 101); break;
    case 102: SIMD_SHUFFLE_EPI32(dst, a, 102); break;
    case 103: SIMD_SHUFFLE_EPI32(dst, a, 103); break;
    case 104: SIMD_SHUFFLE_EPI32(dst, a, 104); break;
    case 105: SIMD_SHUFFLE_EPI32(dst, a, 105); break;
    case 106: SIMD_SHUFFLE_EPI32(dst, a, 106); break;
    case 107: SIMD_SHUFFLE_EPI32(dst, a, 107); break;
    case 108: SIMD_SHUFFLE_EPI32(dst, a, 108); break;
    case 109: SIMD_SHUFFLE_EPI32(dst, a, 109); break;
    case 110: SIMD_SHUFFLE_EPI32(dst, a, 110); break;
    case 111: SIMD_SHUFFLE_EPI32(dst, a, 111); break;
    case 112: SIMD_SHUFFLE_EPI32(dst, a, 112); break;
    case 113: SIMD_SHUFFLE_EPI32(dst, a, 113); break;
    case 114: SIMD_SHUFFLE_EPI32(dst, a, 114); break;
    case 115: SIMD_SHUFFLE_EPI32(dst, a, 115); break;
    case 116: SIMD_SHUFFLE_EPI32(dst, a, 116); break;
    case 117: SIMD_SHUFFLE_EPI32(dst, a, 117); break;
    case 118: SIMD_SHUFFLE_EPI32(dst, a, 118); break;
    case 119: SIMD_SHUFFLE_EPI32(dst, a, 119); break;
    case 120: SIMD_SHUFFLE_EPI32(dst, a, 120); break;
    case 121: SIMD_SHUFFLE_EPI32(dst, a, 121); break;
    case 122: SIMD_SHUFFLE_EPI32(dst, a, 122); break;
    case 123: SIMD_SHUFFLE_EPI32(dst, a, 123); break;
    case 124: SIMD_SHUFFLE_EPI32(dst, a, 124); break;
    case 125: SIMD_SHUFFLE_EPI32(dst, a, 125); break;
    case 126: SIMD_SHUFFLE_EPI32(dst, a, 126); break;
    case 127: SIMD_SHUFFLE_EPI32(dst, a, 127); break;
    case 128: SIMD_SHUFFLE_EPI32(dst, a, 128); break;
    case 129: SIMD_SHUFFLE_EPI32(dst, a, 129); break;
    case 130: SIMD_SHUFFLE_EPI32(dst, a, 130); break;
    case 131: SIMD_SHUFFLE_EPI32(dst, a, 131); break;
    case 132: SIMD_SHUFFLE_EPI32(dst, a, 132); break;
    case 133: SIMD_SHUFFLE_EPI32(dst, a, 133); break;
    case 134: SIMD_SHUFFLE_EPI32(dst, a, 134); break;
    case 135: SIMD_SHUFFLE_EPI32(dst, a, 135); break;
    case 136: SIMD_SHUFFLE_EPI32(dst, a, 136); break;
    case 137: SIMD_SHUFFLE_EPI32(dst, a, 137); break;
    case 138: SIMD_SHUFFLE_EPI32(dst, a, 138); break;
    case 139: SIMD_SHUFFLE_EPI32(dst, a, 139); break;
    case 140: SIMD_SHUFFLE_EPI32(dst, a, 140); break;
    case 141: SIMD_SHUFFLE_EPI32(dst, a, 141); break;
    case 142: SIMD_SHUFFLE_EPI32(dst, a, 142); break;
    case 143: SIMD_SHUFFLE_EPI32(dst, a, 143); break;
    case 144: SIMD_SHUFFLE_EPI32(dst, a, 144); break;
    case 145: SIMD_SHUFFLE_EPI32(dst, a, 145); break;
    case 146: SIMD_SHUFFLE_EPI32(dst, a, 146); break;
    case 147: SIMD_SHUFFLE_EPI32(dst, a, 147); break;
    case 148: SIMD_SHUFFLE_EPI32(dst, a, 148); break;
    case 149: SIMD_SHUFFLE_EPI32(dst, a, 149); break;
    case 150: SIMD_SHUFFLE_EPI32(dst, a, 150); break;
    case 151: SIMD_SHUFFLE_EPI32(dst, a, 151); break;
    case 152: SIMD_SHUFFLE_EPI32(dst, a, 152); break;
    case 153: SIMD_SHUFFLE_EPI32(dst, a, 153); break;
    case 154: SIMD_SHUFFLE_EPI32(dst, a, 154); break;
    case 155: SIMD_SHUFFLE_EPI32(dst, a, 155); break;
    case 156: SIMD_SHUFFLE_EPI32(dst, a, 156); break;
    case 157: SIMD_SHUFFLE_EPI32(dst, a, 157); break;
    case 158: SIMD_SHUFFLE_EPI32(dst, a, 158); break;
    case 159: SIMD_SHUFFLE_EPI32(dst, a, 159); break;
    case 160: SIMD_SHUFFLE_EPI32(dst, a, 160); break;
    case 161: SIMD_SHUFFLE_EPI32(dst, a, 161); break;
    case 162: SIMD_SHUFFLE_EPI32(dst, a, 162); break;
    case 163: SIMD_SHUFFLE_EPI32(dst, a, 163); break;
    case 164: SIMD_SHUFFLE_EPI32(dst, a, 164); break;
    case 165: SIMD_SHUFFLE_EPI32(dst, a, 165); break;
    case 166: SIMD_SHUFFLE_EPI32(dst, a, 166); break;
    case 167: SIMD_SHUFFLE_EPI32(dst, a, 167); break;
    case 168: SIMD_SHUFFLE_EPI32(dst, a, 168); break;
    case 169: SIMD_SHUFFLE_EPI32(dst, a, 169); break;
    case 170: SIMD_SHUFFLE_EPI32(dst, a, 170); break;
    case 171: SIMD_SHUFFLE_EPI32(dst, a, 171); break;
    case 172: SIMD_SHUFFLE_EPI32(dst, a, 172); break;
    case 173: SIMD_SHUFFLE_EPI32(dst, a, 173); break;
    case 174: SIMD_SHUFFLE_EPI32(dst, a, 174); break;
    case 175: SIMD_SHUFFLE_EPI32(dst, a, 175); break;
    case 176: SIMD_SHUFFLE_EPI32(dst, a, 176); break;
    case 177: SIMD_SHUFFLE_EPI32(dst, a, 177); break;
    case 178: SIMD_SHUFFLE_EPI32(dst, a, 178); break;
    case 179: SIMD_SHUFFLE_EPI32(dst, a, 179); break;
    case 180: SIMD_SHUFFLE_EPI32(dst, a, 180); break;
    case 181: SIMD_SHUFFLE_EPI32(dst, a, 181); break;
    case 182: SIMD_SHUFFLE_EPI32(dst, a, 182); break;
    case 183: SIMD_SHUFFLE_EPI32(dst, a, 183); break;
    case 184: SIMD_SHUFFLE_EPI32(dst, a, 184); break;
    case 185: SIMD_SHUFFLE_EPI32(dst, a, 185); break;
    case 186: SIMD_SHUFFLE_EPI32(dst, a, 186); break;
    case 187: SIMD_SHUFFLE_EPI32(dst, a, 187); break;
    case 188: SIMD_SHUFFLE_EPI32(dst, a, 188); break;
    case 189: SIMD_SHUFFLE_EPI32(dst, a, 189); break;
    case 190: SIMD_SHUFFLE_EPI32(dst, a, 190); break;
    case 191: SIMD_SHUFFLE_EPI32(dst, a, 191); break;
    case 192: SIMD_SHUFFLE_EPI32(dst, a, 192); break;
    case 193: SIMD_SHUFFLE_EPI32(dst, a, 193); break;
    case 194: SIMD_SHUFFLE_EPI32(dst, a, 194); break;
    case 195: SIMD_SHUFFLE_EPI32(dst, a, 195); break;
    case 196: SIMD_SHUFFLE_EPI32(dst, a, 196); break;
    case 197: SIMD_SHUFFLE_EPI32(dst, a, 197); break;
    case 198: SIMD_SHUFFLE_EPI32(dst, a, 198); break;
    case 199: SIMD_SHUFFLE_EPI32(dst, a, 199); break;
    case 200: SIMD_SHUFFLE_EPI32(dst, a, 200); break;
    case 201: SIMD_SHUFFLE_EPI32(dst, a, 201); break;
    case 202: SIMD_SHUFFLE_EPI32(dst, a, 202); break;
    case 203: SIMD_SHUFFLE_EPI32(dst, a, 203); break;
    case 204: SIMD_SHUFFLE_EPI32(dst, a, 204); break;
    case 205: SIMD_SHUFFLE_EPI32(dst, a, 205); break;
    case 206: SIMD_SHUFFLE_EPI32(dst, a, 206); break;
    case 207: SIMD_SHUFFLE_EPI32(dst, a, 207); break;
    case 208: SIMD_SHUFFLE_EPI32(dst, a, 208); break;
    case 209: SIMD_SHUFFLE_EPI32(dst, a, 209); break;
    case 210: SIMD_SHUFFLE_EPI32(dst, a, 210); break;
    case 211: SIMD_SHUFFLE_EPI32(dst, a, 211); break;
    case 212: SIMD_SHUFFLE_EPI32(dst, a, 212); break;
    case 213: SIMD_SHUFFLE_EPI32(dst, a, 213); break;
    case 214: SIMD_SHUFFLE_EPI32(dst, a, 214); break;
    case 215: SIMD_SHUFFLE_EPI32(dst, a, 215); break;
    case 216: SIMD_SHUFFLE_EPI32(dst, a, 216); break;
    case 217: SIMD_SHUFFLE_EPI32(dst, a, 217); break;
    case 218: SIMD_SHUFFLE_EPI32(dst, a, 218); break;
    case 219: SIMD_SHUFFLE_EPI32(dst, a, 219); break;
    case 220: SIMD_SHUFFLE_EPI32(dst, a, 220); break;
    case 221: SIMD_SHUFFLE_EPI32(dst, a, 221); break;
    case 222: SIMD_SHUFFLE_EPI32(dst, a, 222); break;
    case 223: SIMD_SHUFFLE_EPI32(dst, a, 223); break;
    case 224: SIMD_SHUFFLE_EPI32(dst, a, 224); break;
    case 225: SIMD_SHUFFLE_EPI32(dst, a, 225); break;
    case 226: SIMD_SHUFFLE_EPI32(dst, a, 226); break;
    case 227: SIMD_SHUFFLE_EPI32(dst, a, 227); break;
    case 228: SIMD_SHUFFLE_EPI32(dst, a, 228); break;
    case 229: SIMD_SHUFFLE_EPI32(dst, a, 229); break;
    case 230: SIMD_SHUFFLE_EPI32(dst, a, 230); break;
    case 231: SIMD_SHUFFLE_EPI32(dst, a, 231); break;
    case 232: SIMD_SHUFFLE_EPI32(dst, a, 232); break;
    case 233: SIMD_SHUFFLE_EPI32(dst, a, 233); break;
    case 234: SIMD_SHUFFLE_EPI32(dst, a, 234); break;
    case 235: SIMD_SHUFFLE_EPI32(dst, a, 235); break;
    case 236: SIMD_SHUFFLE_EPI32(dst, a, 236); break;
    case 237: SIMD_SHUFFLE_EPI32(dst, a, 237); break;
    case 238: SIMD_SHUFFLE_EPI32(dst, a, 238); break;
    case 239: SIMD_SHUFFLE_EPI32(dst, a, 239); break;
    case 240: SIMD_SHUFFLE_EPI32(dst, a, 240); break;
    case 241: SIMD_SHUFFLE_EPI32(dst, a, 241); break;
    case 242: SIMD_SHUFFLE_EPI32(dst, a, 242); break;
    case 243: SIMD_SHUFFLE_EPI32(dst, a, 243); break;
    case 244: SIMD_SHUFFLE_EPI32(dst, a, 244); break;
    case 245: SIMD_SHUFFLE_EPI32(dst, a, 245); break;
    case 246: SIMD_SHUFFLE_EPI32(dst, a, 246); break;
    case 247: SIMD_SHUFFLE_EPI32(dst, a, 247); break;
    case 248: SIMD_SHUFFLE_EPI32(dst, a, 248); break;
    case 249: SIMD_SHUFFLE_EPI32(dst, a, 249); break;
    case 250: SIMD_SHUFFLE_EPI32(dst, a, 250); break;
    case 251: SIMD_SHUFFLE_EPI32(dst, a, 251); break;
    case 252: SIMD_SHUFFLE_EPI32(dst, a, 252); break;
    case 253: SIMD_SHUFFLE_EPI32(dst, a, 253); break;
    case 254: SIMD_SHUFFLE_EPI32(dst, a, 254); break;
    case 255: SIMD_SHUFFLE_EPI32(dst, a, 255); break;
#endif
    default:
        SIMD_SHUFFLE_EPI32(dst, a, 0);
        imm8 = 0;
        break;
    }

    SIMD_STORE_SI(epu8_b, dst);

    for (int i=0; i<lane; ++i) {
        int control = (imm8 >> (2*i)) & 0x3;
        check_abort(epu8_b[i] == epu8_a[control]);
        if (lane != size) {
            check_abort(epu8_b[i+lane] == epu8_a[control+lane]);
        }
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
        test_bitscan_reverse();
        test_bitscan_forward();
        test_load_store_epu();
        test_load_store_ps();
        test_set1();
        test_set();
        test_zeros_ffs();
        test_move_mask_ps();
        test_move_mask_epi8();
        test_add();
        test_alignr_epi8();
        test_shuffle_epi32();
        if (!(count % (CHECK_NUM/10))) {
            printf(".");
            fflush(stdout);
        }
    }
    printf("\nDone\n");
}
