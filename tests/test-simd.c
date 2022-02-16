#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lib/random.h"
#include "lib/simd.h"

#define CHECK_NUM 5000000

#define CREATE_CASE_STATEMENT_1(NUM, COMMAND, ...)          \
    case NUM: COMMAND(__VA_ARGS__, (const int)(NUM)); break;
#define CREATE_CASE_STATEMENT_2(NUM, COMMAND, ...)          \
    CREATE_CASE_STATEMENT_1(NUM, COMMAND, __VA_ARGS__);     \
    CREATE_CASE_STATEMENT_1(NUM+1, COMMAND, __VA_ARGS__);
#define CREATE_CASE_STATEMENT_4(NUM, COMMAND, ...)          \
    CREATE_CASE_STATEMENT_2(NUM, COMMAND, __VA_ARGS__);     \
    CREATE_CASE_STATEMENT_2(NUM+2, COMMAND, __VA_ARGS__);
#define CREATE_CASE_STATEMENT_8(NUM, COMMAND, ...)          \
    CREATE_CASE_STATEMENT_4(NUM, COMMAND, __VA_ARGS__);     \
    CREATE_CASE_STATEMENT_4(NUM+4, COMMAND, __VA_ARGS__);
#define CREATE_CASE_STATEMENT_16(NUM, COMMAND, ...)         \
    CREATE_CASE_STATEMENT_8(NUM, COMMAND, __VA_ARGS__);     \
    CREATE_CASE_STATEMENT_8(NUM+8, COMMAND, __VA_ARGS__);
#define CREATE_CASE_STATEMENT_32(NUM, COMMAND, ...)         \
    CREATE_CASE_STATEMENT_16(NUM, COMMAND, __VA_ARGS__);    \
    CREATE_CASE_STATEMENT_16(NUM+16, COMMAND, __VA_ARGS__);
#define CREATE_CASE_STATEMENT_64(NUM, COMMAND, ...)         \
    CREATE_CASE_STATEMENT_32(NUM, COMMAND, __VA_ARGS__);    \
    CREATE_CASE_STATEMENT_32(NUM+32, COMMAND, __VA_ARGS__);
#define CREATE_CASE_STATEMENT_128(NUM, COMMAND, ...)        \
    CREATE_CASE_STATEMENT_64(NUM, COMMAND, __VA_ARGS__);    \
    CREATE_CASE_STATEMENT_64(NUM+64, COMMAND, __VA_ARGS__);
#define CREATE_CASE_STATEMENT_256(NUM, COMMAND, ...)        \
    CREATE_CASE_STATEMENT_128(NUM, COMMAND, __VA_ARGS__);   \
    CREATE_CASE_STATEMENT_128(NUM+128, COMMAND, __VA_ARGS__);

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
#ifndef NSIMD
    CREATE_CASE_STATEMENT_32(0, SIMD_ALIGNR_EPI8, dst, a, b);
#else
    CREATE_CASE_STATEMENT_4(0, SIMD_ALIGNR_EPI8, dst, a, b);
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
    CREATE_CASE_STATEMENT_256(0, SIMD_SHUFFLE_EPI32, dst, a);
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

static void
test_blend()
{
    uint32_t epu32_a[SIMD_WIDTH];
    uint32_t epu32_b[SIMD_WIDTH];
    uint32_t dst32[SIMD_WIDTH] CACHE_ALIGNED;
    uint64_t epu64_a[SIMD_WIDTH64];
    uint64_t epu64_b[SIMD_WIDTH64];
    uint64_t dst64[SIMD_WIDTH64] CACHE_ALIGNED;
    int imm8_32;
    int imm8_64;

    for (int i=0;i<SIMD_WIDTH;i++) {
        epu32_a[i] = random_uint32();
        epu32_b[i] = random_uint32();
    }
    for (int i=0;i<SIMD_WIDTH64;i++) {
        epu64_a[i] = random_uint64();
        epu64_b[i] = random_uint64();
    }

    imm8_32 = (uint8_t)random_uint32();
    imm8_64 = (uint8_t)random_uint32();

    EPU_REG a32 = SIMD_LOADU_SI(epu32_a);
    EPU_REG b32 = SIMD_LOADU_SI64(epu32_b);
    EPU_REG dst32_reg;

    EPU_REG64 a64 = SIMD_LOADU_SI(epu64_a);
    EPU_REG64 b64 = SIMD_LOADU_SI64(epu64_b);
    EPU_REG64 dst64_reg;

    switch (imm8_32) {
#ifdef __AVX__
    CREATE_CASE_STATEMENT_256(0, SIMD_BLEND_EPI32, dst32_reg, a32, b32);
#elif __SSE__
    CREATE_CASE_STATEMENT_16(0, SIMD_BLEND_EPI32, dst32_reg, a32, b32);
#endif
    default:
        SIMD_BLEND_EPI32(dst32_reg, a32, b32, 0);
        imm8_32 = 0;
        break;
    }

    switch (imm8_64) {
#ifdef __AVX__
    CREATE_CASE_STATEMENT_16(0, SIMD_BLEND_EPI64, dst64_reg, a64, b64);
#elif __SSE__
    CREATE_CASE_STATEMENT_4(0, SIMD_BLEND_EPI64, dst64_reg, a64, b64);
#endif
    default:
        SIMD_BLEND_EPI32(dst64_reg, a64, b64, 0);
        imm8_64 = 0;
        break;
    }

    SIMD_STORE_SI(dst32, dst32_reg);
    SIMD_STORE_SI(dst64, dst64_reg);

    for (int i=0;i<SIMD_WIDTH;i++) {
        if (imm8_32 & 1) {
            check_abort(dst32[i] == epu32_b[i]);
        } else {
            check_abort(dst32[i] == epu32_a[i]);
        }
        imm8_32 >>= 1;
    }

    for (int i=0;i<SIMD_WIDTH64;i++) {
        if (imm8_64 & 1) {
            check_abort(dst64[i] == epu64_b[i]);
        } else {
            check_abort(dst64[i] == epu64_a[i]);
        }
        imm8_64 >>= 1;
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
        test_blend();
        if (!(count % (CHECK_NUM/10))) {
            printf(".");
            fflush(stdout);
        }
    }
    printf("\nDone\n");
}
