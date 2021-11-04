#ifndef _SIMD_H
#define _SIMD_H

#include <stdio.h>

#ifdef __SSE__
# include <x86intrin.h>
#elif __ARM_NEON
# include <arm_neon.h>
#endif

/**
 * @brief General defines that change according to available SIMD engine
 */
#ifdef NSIMD
# define SIMD_WIDTH 1
# define SIMD_WIDTH64 1
# define PS_REG float
# define EPU_REG unsigned int
# define EPU_REG64 unsigned long
#elif __AVX512F__
# define SIMD_WIDTH 16
# define SIMD_WIDTH64 8
# define PS_REG __m512
# define EPU_REG __m512i
# define EPU_REG64 __m512i
# define SIMD_COMMAND(X) _mm512 ## X
# define SIMD_COMMAND_SUFFIX(X) _mm512 ## X ## 512
#elif __AVX__
# define SIMD_WIDTH 8
# define SIMD_WIDTH64 4
# define PS_REG __m256
# define EPU_REG __m256i
# define EPU_REG64 __m256i
# define SIMD_COMMAND(X) _mm256 ## X
# define SIMD_COMMAND_SUFFIX(X) _mm256 ## X ## 256
#elif __SSE__
# define SIMD_WIDTH 4
# define SIMD_WIDTH64 2
# define PS_REG __m128
# define EPU_REG __m128i
# define EPU_REG64 __m128i
# define SIMD_COMMAND(X) _mm ## X
# define SIMD_COMMAND_SUFFIX(X) _mm ## X ## 128
#elif __ARM_NEON
# define SIMD_WIDTH 4
# define SIMD_WIDTH64 2
# define PS_REG float32x4_t
# define EPU_REG uint32x4_t
# define SIMD_COMMAND(X) float32x4_t ## X
#endif

/**
 * @brief Align to 64bytes
 */
#define CACHE_ALIGNED __attribute__ ((aligned (64)))

/**
 * @brief Used to pass information both float and integer vectors
 * @note Must be cached aligned, as AVX loads/stores must be aligned
 */
typedef union {
    float scalars[SIMD_WIDTH];
    unsigned int integers[SIMD_WIDTH];
} simd_vector_t CACHE_ALIGNED;

/**
 * @brief Used for extracting information from 32bit elements
 */
typedef union {
    float f;
    unsigned int d;
} simd_element_t;

/**
 * @brief For clean code of fused multiple add. Computes a=a*b+c
 * @param a vector register
 * @param b vector register
 * @param c vector register
 */
#ifdef NSIMD
# define SIMD_FMA_PS(a,b,c) \
  a = (a*b)+c
#elif __FMA__
# define SIMD_FMA_PS(a,b,c) \
  a = _mm256_fmadd_ps(a, b, c)
#elif __AVX__
# define SIMD_FMA_PS(a,b,c) \
  a = _mm256_mul_ps(a, b);  \
  a = _mm256_add_ps(a, c)
#elif __SSE__
# define SIMD_FMA_PS(a,b,c) \
  a = _mm_mul_ps(a, b);     \
  a = _mm_add_ps(a, c)
#elif __ARM_NEON
# define SIMD_FMA_PS(a,b,c) \
  a = vmlaq_f32(c,b,a)
#endif

/**
 * @brief For clean code of load_ps.
 * @param a memory location, must be aligned
 * (64bit/32bit/16bit, depends on vector width)
 */
#ifdef NSIMD
# define SIMD_LOAD_PS(a) *a
# define SIMD_LOADU_PS(a) *a
# define SIMD_LOADU_PD(a) *a
# define SIMD_LOADU_SI(a) *a
#elif __ARM_NEON
# define SIMD_LOAD_PS(a) vld1q_f32((const float32_t*)a)
# define SIMD_LOADU_PS(a) vld1q_f32((const float32_t*)a)
# define SIMD_LOADU_SI(a) vld1q_u32((const uint32_t*)a)
#else
# define SIMD_LOAD_PS(a) SIMD_COMMAND(_load_ps(a))
# define SIMD_LOADU_PS(a) SIMD_COMMAND(_loadu_ps(a))
# define SIMD_LOADU_PD(a) SIMD_COMMAND(_loadu_pd(a))
# define SIMD_LOADU_PS(a) SIMD_COMMAND(_loadu_ps(a))
# ifdef __AVX512F__
# define SIMD_LOADU_SI(a) _mm512_loadu_si512(a)
# elif __AVX__
# define SIMD_LOADU_SI(a) _mm256_lddqu_si256((const __m256i*)a)
# elif __SSE__
# define SIMD_LOADU_SI(a) _mm_lddqu_si128((const __m128i*)a)
# endif
#endif

/**
 * @brief For clean code of store_ps.
 * @param a memory location, must be aligned
 * (64bit/32bit/16bit, depends on vector width)
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_STORE_PS(a,b) *a = b
# define SIMD_STORE_SI(a,b) *a = b
#elif __ARM_NEON
# define SIMD_STORE_PS(a,b) vst1q_f32((float32_t*)a,b)
#elif __SSE__
# define SIMD_STORE_PS(a,b) SIMD_COMMAND(_store_ps(a,b))
# define SIMD_STORE_SI(a,b) SIMD_COMMAND_SUFFIX(_store_si)((EPU_REG*)a,b)
#endif

/**
 * @brief For clean code of set1.
 * @param a float value
 */
#ifdef NSIMD
# define SIMD_SET1_PS(a) (float)a
# define SIMD_SET1_EPI32(a) (int)a
# define SIMD_SET1_EPI64(a) (long)a
#elif __ARM_NEON
# define SIMD_SET1_PS(a) vdupq_n_f32(a)
# define SIMD_SET1_EPI32(a) vdupq_n_u32(a)
#elif __SSE__
# define SIMD_SET1_PS(a) SIMD_COMMAND(_set1_ps(a))
# define SIMD_SET1_EPI32(a) SIMD_COMMAND(_set1_epi32(a))
# define SIMD_SET1_EPI64(a) SIMD_COMMAND(_set1_epi64x(a))
#endif

/**
 * @brief For clean code of set_epi32.
 * @param ... Integers to put in the vector
 */
#ifdef NSIMD
# define SIMD_SET_PS(a,...) (float)a
# define SIMD_SET_EPI32(a,...) (int)a
# define SIMD_SET_EPI64(a,...) (unsigned long)a
#elif __AVX__
# define SIMD_SET_PS(a,b,c,d,e,f,g,h,...) \
        _mm256_set_ps (a,b,c,d,e,f,g,h)
# define SIMD_SET_EPI32(a,b,c,d,e,f,g,h,...) \
      _mm256_set_epi32(a,b,c,d,e,f,g,h)
# define SIMD_SET_EPI64(a,b,c,d,...) \
      _mm256_set_epi64x(a,b,c,d)
#elif __SSE__
# define SIMD_SET_EPI32(a,b,c,d,...) _mm_set_epi32(a,b,c,d)
# define SIMD_SET_PS(a,b,c,d,...) \
        _mm_set_ps(a,b,c,d,e,f,g,h)
# define SIMD_SET_EPI64(a,b,...) \
        _mm_set_epi64(a,b)
#endif

/**
 * @brief For clean code of zeros vector.
 */
#ifdef NSIMD
# define SIMD_ZEROS_PS 0
# define SIMD_ZEROS_SI 0
#elif __ARM_NEON
# define SIMD_ZEROS_PS vdupq_n_f32(0)
#elif __SSE__
# define SIMD_ZEROS_PS SIMD_COMMAND(_setzero_ps())
# define SIMD_ZEROS_SI SIMD_COMMAND_SUFFIX(_setzero_si)()
#endif

/**
 * @brief For clean code of subtract.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_SUB_PS(a,b) a-b
# define SIMD_SUB_EPI32(a,b) a-b
# define SIMD_SUB_EPI64(a,b) a-b
#elif __ARM_NEON
# define SIMD_SUB_PS(a,b) vsubq_f32(a,b)
#elif __SSE__
# define SIMD_SUB_PS(a,b) SIMD_COMMAND(_sub_ps(a, b))
# define SIMD_SUB_EPI32(a,b) SIMD_COMMAND(_sub_epi32(a, b))
# define SIMD_SUB_EPI64(a,b) SIMD_COMMAND(_sub_epi64(a, b))
#endif

# define SIMD_SUB_SI(a,b) SIMD_SUB_EPI32(a,b)

/**
 * @brief For clean code of addition.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_ADD_PS(a,b) (a+b)
# define SIMD_ADD_EPI32(a,b) (a+b)
# define SIMD_ADD_EPI64(a,b) (a+b)
#elif __ARM_NEON
# define SIMD_ADD_PS(a,b) vaddq_f32(a,b)
#elif __SSE__
# define SIMD_ADD_PS(a,b) SIMD_COMMAND(_add_ps(a, b))
# define SIMD_ADD_EPI32(a,b) SIMD_COMMAND(_add_epi32(a, b))
# define SIMD_ADD_EPI64(a,b) SIMD_COMMAND(_add_epi64(a, b))
#endif

# define SIMD_ADD_SI(a,b) SIMD_ADD_EPI32(a,b)

/**
 * @brief For clean code of shift.
 * @param a output, vector register
 * @param b vector register
 * @param c bit count
 */
#ifdef NSIMD
# define SIMD_SRL_EPU32(a,b,c) a=(b>>c)
# define SIMD_SRL_EPU64(a,b,c) a=(b>>c)
# define SIMD_SRA_EPI32(a,b,c) a=(b>>c)
# define SIMD_SRA_EPI64(a,b,c) a=(b>>c)
# define SIMD_SLL_EPU32(a,b,c) a=(b<<c)
# define SIMD_SLL_EPU64(a,b,c) a=(b<<c)
#elif __SSE__
# define SIMD_SRL_EPU32(a,b,c)              \
    {                                       \
    __m128i count;                          \
    count = _mm_set_epi64x(0, c);           \
    a=SIMD_COMMAND(_srl_epi32(b, count));   \
    }
# define SIMD_SRL_EPU64(a,b,c)              \
    {                                       \
    __m128i count;                          \
    count = _mm_set_epi64x(0, c);           \
    a=SIMD_COMMAND(_srl_epi64(b, count));   \
    }
# define SIMD_SRL_EPI32(a,b,c)              \
    {                                       \
    __m128i count;                          \
    count = _mm_set_epi64x(0, c);           \
    a=SIMD_COMMAND(_sra_epi32(b, count));   \
    }
# define SIMD_SRL_EPI64(a,b,c)              \
    {                                       \
    __m128i count;                          \
    count = _mm_set_epi64x(0, c);           \
    a=SIMD_COMMAND(_sra_epi64(b, count));   \
    }
# define SIMD_SLL_EPI32(a,b,c)              \
    {                                       \
    __m128i count;                          \
    count = _mm_set_epi64x(0, c);           \
    a=SIMD_COMMAND(_sll_epi32(b, count));   \
    }
# define SIMD_SLL_EPI64(a,b,c)              \
    {                                       \
    __m128i count;                          \
    count = _mm_set_epi64x(0, c);           \
    a=SIMD_COMMAND(_sll_epi64(b, count));   \
    }
#endif


/**
 * @brief For clean code of divide.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_DIV_PS(a,b) a/b
#elif __ARM_NEON
# define SIMD_DIV_PS(a,b) vdivq_f32(a,b)
#elif __SSE__
# define SIMD_DIV_PS(a,b) SIMD_COMMAND(_div_ps(a, b))
#endif

/**
 * @brief For clean code of multiply.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_MUL_PS(a,b) a*b
# define SIMD_MUL_EPI32(a,b) a*b
#elif __ARM_NEON
# define SIMD_MUL_PS(a,b) vmulq_f32(a,b)
#elif __SSE__
# define SIMD_MUL_PS(a,b) SIMD_COMMAND(_mul_ps(a, b))
# define SIMD_MUL_EPI32(a,b) SIMD_COMMAND(_mul_epi32(a, b))
#endif

/**
 * @brief For clean code of and/or/and-not (~a&b)
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
inline float
simd_helper_and_ps__(float a, float b)
{
    simd_element_t x, y, z;
    x.f = a; y.f = b;
    z.d=x.d & y.d;
    return z.f;
}
inline float
simd_helper_or_ps__(float a, float b)
{
    simd_element_t x, y, z;
    x.f = a; y.f = b;
    z.d=x.d | y.d;
    return z.f;
}
inline float
simd_helper_andnot_ps__(float a, float b)
{
    simd_element_t x, y, z;
    x.f = a; y.f = b;
    z.d=(~x.d) & ~y.d;
    return z.f;
}
# define SIMD_AND_PS(a,b) simd_helper_and_ps__(a, b)
# define SIMD_AND_SI(a,b) (a&b)
# define SIMD_OR_PS(a,b) simd_helper_or_ps__(a, b)
# define SIMD_OR_SI(a,b) (a|b)
# define SIMD_ANDNOT_PS(a,b) simd_helper_andnot_ps__(a, b)
# define SIMD_ANDNOT_SI(a,b) (~a&b)
#elif __ARM_NEON
# define SIMD_AND_PS(a,b)                                      \
    vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(a),  \
                                    vreinterpretq_s32_f32(b))) 
#elif __SSE__
# define SIMD_AND_PS(a,b) SIMD_COMMAND(_and_ps(a, b))
# define SIMD_AND_SI(a,b) SIMD_COMMAND_SUFFIX(_and_si)(a, b)
# define SIMD_OR_PS(a,b) SIMD_COMMAND(_or_ps(a, b))
# define SIMD_OR_SI(a,b) SIMD_COMMAND_SUFFIX(_or_si)(a, b)
# define SIMD_ANDNOT_PS(a,b) SIMD_COMMAND(_andnot_ps(a, b))
# define SIMD_ANDNOT_SI(a,b) SIMD_COMMAND_SUFFIX(_andnot_si)(a, b)
#endif

/**
 * @brief For clean code of max/min (float32)
 * @param a output register
 * @param b vector register
 * @param c vector register
 */
#ifdef NSIMD
# define SIMD_MAX_PS(a,b,c) a=(b>c) ? b : c
# define SIMD_MIN_PS(a,b,c) a=(b>c) ? c : b
#elif __AVX512F__
# define SIMD_MAX_PS(a,b,c) a=_mm512_max_ps(b,c)
# define SIMD_MIN_PS(a,b,c) a=_mm512_min_ps(b,c)
#elif __AVX__
# define SIMD_MAX_PS(a,b,c) a=_mm256_max_ps(b,c)
# define SIMD_MIN_PS(a,b,c) a=_mm256_min_ps(b,c)
#elif __SSE__
# define SIMD_MAX_PS(a,b,c) a=_mm_max_ps(b,c)
# define SIMD_MIN_PS(a,b,c) a=_mm_min_ps(b,c)
#elif __ARM_NEON
# define SIMD_MAX_PS(a,b,c) a=vmaxq_f32(b,c)
# define SIMD_MIN_PS(a,b,c) a=vminq_f32(b,c)
#endif

/**
 * @brief For clean code of max/min (uint32)
 * @param a output register
 * @param b vector register
 * @param c vector register
 */
#ifdef NSIMD
# define SIMD_MAX_EPU32(a,b,c) a=(b>c) ? b : c
# define SIMD_MIN_EPU32(a,b,c) a=(b>c) ? c : b
# define SIMD_MAX_EPI32(a,b,c) a=(b>c) ? b : c
# define SIMD_MIN_EPI32(a,b,c) a=(b>c) ? c : b
#elif __AVX512F__
# define SIMD_MAX_EPU32(a,b,c) a=_mm512_max_epu32(b,c)
# define SIMD_MIN_EPU32(a,b,c) a=_mm512_min_epu32(b,c)
# define SIMD_MAX_EPI32(a,b,c) a=_mm512_max_epi32(b,c)
# define SIMD_MIN_EPI32(a,b,c) a=_mm512_min_epi32(b,c)
#elif __AVX2__
# define SIMD_MAX_EPU32(a,b,c) a=_mm256_max_epu32(b,c)
# define SIMD_MIN_EPU32(a,b,c) a=_mm256_min_epu32(b,c)
# define SIMD_MAX_EPI32(a,b,c) a=_mm256_max_epi32(b,c)
# define SIMD_MIN_EPI32(a,b,c) a=_mm256_min_epi32(b,c)
#elif __AVX__
# define SIMD_MAX_EPU32(a,b,c)                                               \
    {                                                                        \
    __m128i b_low = _mm256_extractf128_si256(b, 1);                          \
    __m128i b_high = _mm256_castsi256_si128(b);                              \
    __m128i c_low = _mm256_extractf128_si256(c, 1);                          \
    __m128i c_high = _mm256_castsi256_si128(c);                              \
    __m128i low_res = _mm_max_epu32(b_low, c_low);                           \
    __m128i high_res = _mm_max_epu32(b_high, c_high);                        \
    __m128  low_res_ps = _mm_castsi128_ps(low_res);                          \
    __m128  high_res_ps =_mm_castsi128_ps(high_res);                         \
    __m256  high_res_wide_ps = _mm256_castps128_ps256(high_res_ps);          \
    __m256  result_ps = _mm256_insertf128_ps(high_res_wide_ps,low_res_ps,1); \
    a=_mm256_castps_si256(result_ps);                                        \
    }
# define SIMD_MIN_EPU32(a,b,c)                                               \
    {                                                                        \
    __m128i b_low = _mm256_extractf128_si256(b, 1);                          \
    __m128i b_high = _mm256_castsi256_si128(b);                              \
    __m128i c_low = _mm256_extractf128_si256(c, 1);                          \
    __m128i c_high = _mm256_castsi256_si128(c);                              \
    __m128i low_res = _mm_min_epu32(b_low, c_low);                           \
    __m128i high_res = _mm_min_epu32(b_high, c_high);                        \
    __m128  low_res_ps = _mm_castsi128_ps(low_res);                          \
    __m128  high_res_ps =_mm_castsi128_ps(high_res);                         \
    __m256  high_res_wide_ps = _mm256_castps128_ps256(high_res_ps);          \
    __m256  result_ps = _mm256_insertf128_ps(high_res_wide_ps,low_res_ps,1); \
    a=_mm256_castps_si256(result_ps);                                        \
    }
# define SIMD_MAX_EPI32(a,b,c)                                               \
    {                                                                        \
    __m128i b_low = _mm256_extractf128_si256(b, 1);                          \
    __m128i b_high = _mm256_castsi256_si128(b);                              \
    __m128i c_low = _mm256_extractf128_si256(c, 1);                          \
    __m128i c_high = _mm256_castsi256_si128(c);                              \
    __m128i low_res = _mm_max_epi32(b_low, c_low);                           \
    __m128i high_res = _mm_max_epi32(b_high, c_high);                        \
    __m128  low_res_ps = _mm_castsi128_ps(low_res);                          \
    __m128  high_res_ps =_mm_castsi128_ps(high_res);                         \
    __m256  high_res_wide_ps = _mm256_castps128_ps256(high_res_ps);          \
    __m256  result_ps = _mm256_insertf128_ps(high_res_wide_ps,low_res_ps,1); \
    a=_mm256_castps_si256(result_ps);                                        \
    }
# define SIMD_MIN_EPI32(a,b,c)                                               \
    {                                                                        \
    __m128i b_low = _mm256_extractf128_si256(b, 1);                          \
    __m128i b_high = _mm256_castsi256_si128(b);                              \
    __m128i c_low = _mm256_extractf128_si256(c, 1);                          \
    __m128i c_high = _mm256_castsi256_si128(c);                              \
    __m128i low_res = _mm_min_epi32(b_low, c_low);                           \
    __m128i high_res = _mm_min_epi32(b_high, c_high);                        \
    __m128  low_res_ps = _mm_castsi128_ps(low_res);                          \
    __m128  high_res_ps =_mm_castsi128_ps(high_res);                         \
    __m256  high_res_wide_ps = _mm256_castps128_ps256(high_res_ps);          \
    __m256  result_ps = _mm256_insertf128_ps(high_res_wide_ps,low_res_ps,1); \
    a=_mm256_castps_si256(result_ps);                                        \
    }
#elif __SSE__
# define SIMD_MAX_EPU32(a,b,c) a=_mm_max_epu32(b,c)
# define SIMD_MIN_EPU32(a,b,c) a=_mm_min_epu32(b,c)
# define SIMD_MAX_EPI32(a,b,c) a=_mm_max_epi32(b,c)
# define SIMD_MIN_EPI32(a,b,c) a=_mm_min_epi32(b,c)
#elif __ARM_NEON
# define SIMD_MAX_EPU32(a,b,c) a=vmaxq_u32(b,c)
# define SIMD_MIN_EPU32(a,b,c) a=vminq_u32(b,c)
#endif

/**
 * @brief For clean code of casting float vector to integer vector
 * @param a vector register
 */
#ifdef NSIMD

inline unsigned int
simd_helper_castps_si__(float a)
{
    simd_element_t x;
    x.f = a;
    return x.d;
}

# define SIMD_CASTPS_SI(a) simd_helper_castps_si__(a)
#elif __AVX512F__
# define SIMD_CASTPS_SI(a) _mm512_castps_si512(a)
#elif __AVX__
# define SIMD_CASTPS_SI(a) _mm256_castps_si256(a)
#elif __SSE__
# define SIMD_CASTPS_SI(a) _mm_castps_si128(a)
#elif __ARM_NEON
# define SIMD_CASTPS_SI(a) vreinterpretq_u32_f32(a)
#endif

/**
 * @brief For clean code of casting integer vector to float vector
 * @param a vector register
 */
#ifdef NSIMD

inline float
simd_helper_castsi_ps__(unsigned int a)
{
    simd_element_t x;
    x.d = a;
    return x.f;
}

# define SIMD_CASTSI_PS(a) simd_helper_castsi_ps__(a)
#elif __AVX512F__
# define SIMD_CASTSI_PS(a) _mm512_castsi512_ps(a)
#elif __AVX__
# define SIMD_CASTSI_PS(a) _mm256_castsi256_ps(a)
#elif __SSE__
# define SIMD_CASTSI_PS(a) _mm_castsi128_ps(a)
#elif __ARM_NEON
# define SIMD_CASTSI_PS(a) vreinterpretq_f32_u32(a)
#endif

/**
 * @brief Compares each float in a vector
 * @param a output result (0xffffffff / 0x0) per float
 * @param b float vector register
 * @param c float vector register
 */
#ifdef NSIMD
# define SIMD_CMPEQ_PS(a,b,c) a=(b==c) ? 0xffffffff : 0x0
# define SIMD_CMPNEQ_PS(a,b,c) a=(b!=c) ? 0xffffffff : 0x0
#elif __AVX__
# define SIMD_CMPEQ_PS(a,b,c) a=_mm256_cmp_ps(b,c, _CMP_EQ_UQ)
# define SIMD_CMPNEQ_PS(a,b,c) a=_mm256_cmp_ps(b,c, _CMP_NEQ_UQ)
#elif __SSE__
# define SIMD_CMPEQ_PS(a,b,c) a=_mm_cmpeq_ps(b,c)
# define SIMD_CMPNEQ_PS(a,b,c) a=_mm_cmpneq_ps(b,c)
#elif __ARM_NEON
# define SIMD_CMPEQ_PS(a,b,c) \
    a=vreinterpretq_f32_u32(vceqq_f32(b,c))
# define SIMD_CMPNEQ_PS(a,b,c) \
    a=vreinterpretq_f32_u32(vmvnq_u32(vceqq_f32(b,c)))
#endif

/**
 * @brief For clean code of compare equal (integers).
 * @param a output result (0xffffffff / 0x0) per integer
 * @param b integer vector register
 * @param c integer vector register
 */
#ifdef NSIMD
# define SIMD_CMPEQ_EPI32(a,b,c) a=(b==c) ? 0xffffffff : 0x0
#elif __AVX512F__
# define SIMD_CMPEQ_EPI32(a,b,c)                                           \
    {                                                                      \
    __m256i b_low = _mm512_extracti32x8_epi32(b, 1);                       \
    __m256i b_high = _mm512_extracti32x8_epi32(b, 0);                      \
    __m256i c_low = _mm512_extracti32x8_epi32(c, 1);                       \
    __m256i c_high = _mm512_extracti32x8_epi32(b, 0);                      \
    __m256i low_res = _mm256_cmpeq_epi32(b_low, c_low);                    \
    __m256i high_res = _mm256_cmpeq_epi32(b_high, c_high);                 \
    __m256  low_res_ps = _mm256_castsi256_ps(low_res);                     \
    __m256  high_res_ps =_mm256_castsi256_ps(high_res);                    \
    __m512  high_res_wide_ps = _mm512_castps256_ps512(high_res_ps);        \
    __m512  result_ps = _mm512_insertf32x8(high_res_wide_ps,low_res_ps,1); \
    a=_mm512_castps_si512(result_ps);                                      \
    }
#elif __AVX2__
# define SIMD_CMPEQ_EPI32(a,b,c) a=_mm256_cmpeq_epi32(b,c)
#elif __AVX__
# define SIMD_CMPEQ_EPI32(a,b,c)                                             \
    {                                                                        \
    __m128i b_low = _mm256_extractf128_si256(b, 1);                          \
    __m128i b_high = _mm256_extractf128_si256(b, 0);                         \
    __m128i c_low = _mm256_extractf128_si256(c, 1);                          \
    __m128i c_high = _mm256_extractf128_si256(c, 0);                         \
    __m128i low_res = _mm_cmpeq_epi32(b_low, c_low);                         \
    __m128i high_res = _mm_cmpeq_epi32(b_high, c_high);                      \
    __m128  low_res_ps = _mm_castsi128_ps(low_res);                          \
    __m128  high_res_ps =_mm_castsi128_ps(high_res);                         \
    __m256  high_res_wide_ps = _mm256_castps128_ps256(high_res_ps);          \
    __m256  result_ps = _mm256_insertf128_ps(high_res_wide_ps,low_res_ps,1); \
    a=_mm256_castps_si256(result_ps);                                        \
    }
#elif __SSE__
# define SIMD_CMPEQ_EPI32(a,b,c) a=_mm_cmpeq_epi32(b,c)
#elif __ARM_NEON
# define SIMD_CMPEQ_EPI32(a,b,c) a=vceqq_u32(b,c)
#endif

/**
 * @brief Compare greater than (b>c) of signed integers
 * @param a Output
 * @param b Signed integer
 * @param c Signed integer
 */
#ifdef NSIMD
# define SIMD_CMPGT_EPI8(a,b,c) (a=(b>c))
# define SIMD_CMPGT_EPI16(a,b,c) (a=(b>c))
# define SIMD_CMPGT_EPI32(a,b,c) (a=(b>c))
# define SIMD_CMPGT_EPI64(a,b,c) (a=(b>c))
#elif __AVX2__
# define SIMD_CMPGT_EPI8(a,b,c)  a=_mm256_cmpgt_epi8(b,c)
# define SIMD_CMPGT_EPI16(a,b,c) a=_mm256_cmpgt_epi16(b,c)
# define SIMD_CMPGT_EPI32(a,b,c) a=_mm256_cmpgt_epi32(b,c)
# define SIMD_CMPGT_EPI64(a,b,c) a=_mm256_cmpgt_epi64(b,c)
#elif __SSE__
# define SIMD_CMPGT_EPI8(a,b,c) a= _mm_cmpgt_epi8(b,c)
# define SIMD_CMPGT_EPI16(a,b,c) a=_mm_cmpgt_epi16(b,c)
# define SIMD_CMPGT_EPI32(a,b,c) a=_mm_cmpgt_epi32(b,c)
# define SIMD_CMPGT_EPI64(a,b,c) a=_mm_cmpgt_epi64(b,c)
#endif

/**
 * @brief Compare greater than (a>=b) of unsigned integers
 * @param a Output
 * @param b Unsigned integer
 * @param c Unsigned integer
 */
#ifdef NSIMD
# define SIMD_CMPGE_EPU8(a,b,c) (a=(b>c))
# define SIMD_CMPGE_EPU16(a,b,c) (a=(b>c))
# define SIMD_CMPGE_EPU32(a,b,c) (a=(b>c))
# define SIMD_CMPGE_EPU64(a,b,c) (a=(b>c))
#elif __AVX2__
# define __SIMD_CMPGT_HELPER(a,b,c,SIZE)        \
    {                                           \
    __m256i max = _mm256_max_epu ## SIZE(b,c);  \
    a=_mm256_cmpeq_epi ## SIZE(b,max);          \
    }
# define SIMD_CMPGE_EPU8(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,8)
# define SIMD_CMPGE_EPU16(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,16)
# define SIMD_CMPGE_EPU32(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,32)
# define SIMD_CMPGE_EPU64(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,64)
#elif __SSE__
# define __SIMD_CMPGT_HELPER(a,b,c,SIZE)        \
    {                                           \
    __m128i max = _mm_max_epu ## SIZE(b,c);     \
    a=_mm_cmpeq_epi ## SIZE(b,max);             \
    }
# define SIMD_CMPGE_EPU8(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,8)
# define SIMD_CMPGE_EPU16(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,16)
# define SIMD_CMPGE_EPU32(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,32)
# define SIMD_CMPGE_EPU64(a,b,c) __SIMD_CMPGT_HELPER(a,b,c,64)
#endif

/**
 * @brief Set each bit of mask a based on the most significant bit
 * of the corresponding packed single-precision (32-bit)
 * floating-point element in b
 */
#ifdef NSIMD
# define SIMD_MOVE_MASK_PS(a,b) a=(b!=0)
#elif __AVX__
# define SIMD_MOVE_MASK_PS(a,b) a=_mm256_movemask_ps(b)
#elif __SSE__
# define SIMD_MOVE_MASK_PS(a,b) a=_mm_movemask_ps(b)
#elif __ARM_NEON
#define SIMD_MOVE_MASK_PS(a, b)                                       \
    {                                                                 \
      uint32_t MSB_select = 0x80000000;                               \
      uint32x4_t uintTest = vdupq_n_u32(MSB_select);                  \
      uint32x4_t res = vandq_u32(vreinterpretq_u32_f32(b),uintTest);  \
      uint32x2_t hiDual = vget_high_u32(res);                         \
      uint32x2_t loDual = vget_low_u32(res);                          \
      a = ((vget_lane_u32(loDual,0) == 0x80000000))      +            \
          ((vget_lane_u32(loDual,1) == 0x80000000) << 1) +            \
          ((vget_lane_u32(hiDual,0) == 0x80000000) << 2) +            \
          ((vget_lane_u32(hiDual,1) == 0x80000000) << 3)              \
    }
#endif

#ifdef __SSE__

/**
 * @brief Reduces sum of 128bit vector of floats to 32bit float
 * @param a output 32bit float
 * @param b input 128bit float
 */
#define __SIMD_REDUCE_SUM_128_TO_32_PS(a, b)           \
    {                                                  \
    __m128 loDual = b;                                 \
    __m128 hiDual = _mm_movehl_ps(loDual, loDual);     \
    __m128 sumDual = _mm_add_ps(loDual, hiDual);       \
    __m128 lo = sumDual;                               \
    __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1); \
    __m128 sum = _mm_add_ss(lo, hi);                   \
    a = _mm_cvtss_f32(sum);                            \
    }
/**
 * @brief Reduces sum of 128bit vector of floats to 32bit integer
 * @param a output 32bit integer
 * @param b input 128bit integer
 */
#define __SIMD_REDUCE_SUM_128_TO_32_EPI32(a, b)        \
    {                                                  \
    __m128i x = _mm_hadd_epi32(b,b);                   \
    __m128i y = _mm_hadd_epi32(x,x);                   \
    a = _mm_extract_epi32(y, 0);                       \
    }

/**
 * @brief Reduces sum of 128bit vector of floats to 64bit integer
 * @param a output 32bit integer
 * @param b input 128bit integer
 */
#define __SIMD_REDUCE_SUM_128_TO_64_EPI64(a, b)        \
    {                                                  \
    long x = _mm_extract_epi64 (b,0);                  \
    long y = _mm_extract_epi64 (b,1);                  \
    a = x+y;                                           \
    }

/**
 * @brief Reduces sum of 256bit vector of floats to 128bit vector of floats
 * @param a output 128bit float
 * @param b input 256bit float
 */
#define __SIMD_REDUCE_SUM_256_TO_128_PS(a, b)    \
    {                                            \
    __m128 hiQuad = _mm256_extractf128_ps(b, 1); \
    __m128 loQuad = _mm256_castps256_ps128(b);   \
    a = _mm_add_ps(loQuad, hiQuad);              \
    }

/**
 * @brief Reduces sum of 256bit vector of floats to 128bit vector of integer
 * @param a output 128bit integer
 * @param b input 256bit integer
 */
#define __SIMD_REDUCE_SUM_256_TO_128_EPI32(a, b)    \
    {                                               \
    __m128i hiQuad = _mm256_extractf128_si256(b, 1);\
    __m128i loQuad = _mm256_castsi256_si128 (b);    \
    a = _mm_add_epi32(loQuad, hiQuad);              \
    }

/**
 * @brief Reduces sum of 256bit vector of floats to 128bit vector of integer
 * @param a output 128bit integer
 * @param b input 256bit integer
 */
#define __SIMD_REDUCE_SUM_256_TO_128_EPI64(a, b)    \
    {                                               \
    __m128i hiQuad = _mm256_extractf128_si256(b, 1);\
    __m128i loQuad = _mm256_castsi256_si128 (b);    \
    a = _mm_add_epi64(loQuad, hiQuad);              \
    }

#elif __ARM_NEON

/**
 * @brief Reduces sum of 128bit vector of floats to 32bit float
 * @param a output 32bit float
 * @param b input 128bit float
 */
#define __SIMD_REDUCE_SUM_128_TO_32_PS(a, b)           \
    {                                                  \
    float32x2_t hiDual = vget_high_f32(b);             \
    float32x2_t loDual = vget_low_f32(b);              \
    float32x2_t sumDual = vadd_f32(loDual, hiDual);    \
    float32_t hi = vget_lane_f32(sumDual, 1);          \
    float32_t lo = vget_lane_f32(sumDual, 0);          \
    a = lo + hi;                                       \
    }

#endif


/**
 * @brief Reduces SUM of all floats in vector register into a single float
 * @param a output of 32bit float
 * @param b input register vector
 */
#ifdef NSIMD
# define SIMD_REDUCE_SUM_PS(a, b) a=b
# define SIMD_REDUCE_SUM_EPI32(a, b) a=b
# define SIMD_REDUCE_SUM_EPI64(a, b) a=b
#elif __AVX__
# define SIMD_REDUCE_SUM_PS(a, b)                \
    {                                            \
    __m128 imm0;                                 \
    __SIMD_REDUCE_SUM_256_TO_128_PS(imm0, b);    \
    __SIMD_REDUCE_SUM_128_TO_32_PS(a, imm0);     \
    }
# define SIMD_REDUCE_SUM_EPI32(a, b)             \
    {                                            \
    __m128i imm0;                                \
    __SIMD_REDUCE_SUM_256_TO_128_EPI32(imm0, b); \
    __SIMD_REDUCE_SUM_128_TO_32_EPI32(a, imm0);  \
    }
# define SIMD_REDUCE_SUM_EPI64(a, b)             \
    {                                            \
    __m128i imm0;                                \
    __SIMD_REDUCE_SUM_256_TO_128_EPI64(imm0, b); \
    __SIMD_REDUCE_SUM_128_TO_64_EPI64(a, imm0);  \
    }
#elif __SSE__
# define SIMD_REDUCE_SUM_PS(a, b) \
    __SIMD_REDUCE_SUM_128_TO_32_PS(a, b);
# define SIMD_REDUCE_SUM_EPI32(a, b) \
    __SIMD_REDUCE_SUM_128_TO_32_EPI32(a, b);
# define SIMD_REDUCE_SUM_EPI64(a, b) \
    __SIMD_REDUCE_SUM_128_TO_64_EPI64(a, b);
#elif __ARM_NEON
# define SIMD_REDUCE_SUM_PS(a,b) \
    __SIMD_REDUCE_SUM_128_TO_32_PS(a,b)
#endif


#ifdef __SSE__

/**
 * @brief Reduces max of 128bit vector of floats to 32bit float
 * @param a output 32bit float
 * @param b input 128bit float
 */
#define __SIMD_REDUCE_MAX_128_TO_32_PS(a, b)           \
    {                                                  \
    __m128 loDual = b;                                 \
    __m128 hiDual = _mm_movehl_ps(loDual, loDual);     \
    __m128 maxDual = _mm_max_ps(loDual, hiDual);       \
    __m128 lo = maxDual;                               \
    __m128 hi = _mm_shuffle_ps(maxDual, maxDual, 0x1); \
    __m128 sum = _mm_max_ps(lo, hi);                   \
    a = _mm_cvtss_f32(sum)                             \
    }

/**
 * @brief Reduces max of 256bit vector of floats to 128bit vector of floats
 * @param a output 128bit float
 * @param b input 256bit float
 */
#define __SIMD_REDUCE_MAX_256_TO_128_PS(a, b)    \
    {                                            \
    __m128 hiQuad = _mm256_extractf128_ps(b, 1); \
    __m128 loQuad = _mm256_castps256_ps128(b);   \
    a = _mm_max_ps(loQuad, hiQuad)               \
    }

/**
 * @brief Reduces max of 512bit vector of floats to 256bit vector of floats
 * @param a output 256bit float
 * @param b input 512bit float
 */
#define __SIMD_REDUCE_MAX_512_TO_256_PS(a, b)     \
    {                                             \
    __m256 hiOcta = _mm512_extractf32x8_ps(b, 1); \
    __m256 loOcta = _mm512_castps512_ps256(b);    \
    a = _mm256_max_ps(loOcta, hiOcta);            \
    }

#elif __ARM_NEON

/**
 * @brief Reduces max of 128bit vector of floats to 32bit float
 * adapted for ARM NEON architecture
 * @param a output 32bit float
 * @param b input 128bit float
 */
#define __SIMD_REDUCE_MAX_128_TO_32_PS(a, b)               \
    {                                                      \
    float32x2_t hiDual = vget_high_f32(b);                 \
    float32x2_t loDual = vget_low_f32(b);                  \
    float32x2_t maxDual = vpmax_f32(loDual, hiDual);       \
    float32x2_t maxOfMaxDual = vpmax_f32(maxDual, maxDual);\
    a = vget_lane_f32(maxOfMaxDual, 0);                    \
    }

#endif

/**
 * @brief Reduces MAX of all floats in vector register into a single float
 * @param a output of 32bit float
 * @param b input register vector
 */
#ifdef NSIMD
# define SIMD_REDUCE_MAX_PS(a, b) a=b
#elif __AVX512F__
# define SIMD_REDUCE_MAX_PS(a, b)                \
    {                                            \
    __m256 imm0;                                 \
    __m128 imm1;                                 \
    __SIMD_REDUCE_MAX_512_TO_256_PS(imm0, b);    \
    __SIMD_REDUCE_MAX_256_TO_128_PS(imm1, imm0); \
    __SIMD_REDUCE_MAX_128_TO_32_PS(a, imm1);     \
    }
#elif __AVX__
# define SIMD_REDUCE_MAX_PS(a, b)                \
    {                                            \
    __m128 imm0;                                 \
    __SIMD_REDUCE_MAX_256_TO_128_PS(imm0, b);    \
    __SIMD_REDUCE_MAX_128_TO_32_PS(a, imm0);     \
    }
#elif __SSE__
# define SIMD_REDUCE_MAX_PS(a, b)                \
    __SIMD_REDUCE_MAX_128_TO_32_PS(a, b);
#elif __ARM_NEON
# define SIMD_REDUCE_MAX_PS(a,b)                 \
    SIMD_REDUCE_MAX_128_TO_32_PS(a,b)
#endif


#ifdef __SSE__

/**
 * @brief Reduces max of 128bit vector of uint32 to uint32/64
 * @param a output 32bit float
 * @param b input 128bit float
 */
#define __SIMD_REDUCE_MAX_128_TO_32_EPU32(a, b)      \
   {                                                 \
    __m128i loDual = b;                              \
    __m128 loCast = _mm_castsi128_ps(loDual);        \
    __m128 hiCast = _mm_movehl_ps(loCast, loCast);   \
    __m128i hiDual = _mm_castps_si128(hiCast);       \
    __m128i maxDual = _mm_max_epu32(loDual, hiDual); \
    __m128i lo = maxDual;                            \
    __m128i hi = _mm_shuffle_epi32(maxDual, 0x1);    \
    __m128i max = _mm_max_epu32(lo, hi);             \
    a = _mm_cvtsi128_si32(max);                      \
   }

#define __SIMD_REDUCE_MAX_128_TO_64_EPU64(a, b)      \
   {                                                 \
    unsigned long x = _mm_extract_epi64(b,0);        \
    unsigned long y = _mm_extract_epi64(b,1);        \
    unsigned long flag = (x>y)*-1;                   \
    a = (flag&x)|((~flag)&y);                        \
   }

/**
 * @brief Reduces max of 256bit vector of floats to 128bit vector of floats
 * @param a output 128bit float
 * @param b input 256bit float
 */
#define __SIMD_REDUCE_MAX_256_TO_128_EPU32(a, b)     \
    {                                                \
    __m128i hiQuad = _mm256_extractf128_si256(b, 1); \
    __m128i loQuad = _mm256_castsi256_si128(b);      \
    a = _mm_max_epu32(loQuad, hiQuad);               \
    }

#define __SIMD_REDUCE_MAX_256_TO_64_EPU64(a, b)              \
    __m128i hiQuad = _mm256_extractf128_si256(b, 1);         \
    __m128i loQuad = _mm256_castsi256_si128(b);              \
    unsigned long maxhi, maxlo, flag;                        \
    __SIMD_REDUCE_MAX_128_TO_64_EPU64(maxhi,hiQuad);         \
    __SIMD_REDUCE_MAX_128_TO_64_EPU64(maxlo,loQuad);         \
    flag = (maxhi>maxlo)*-1;                                 \
    a = (flag&maxhi)|((~flag)&maxlo);                        \

#elif __ARM_NEON

/**
 * @brief Reduces max of 128bit vector of uint32 to uint32
 * adapted for ARM NEON architecture
 * @param a output 32bit float
 * @param b input 128bit float
 */
#define __SIMD_REDUCE_MAX_128_TO_32_EPU32_ARM(a,b)          \
   {                                                        \
    uint32x2_t hiDual = vget_high_u32(b);                   \
    uint32x2_t loDual = vget_low_u32(b);                    \
    uint32x2_t maxDual = vpmax_u32(loDual, hiDual);         \
    uint32x2_t maxOfMaxDual = vpmax_u32(maxDual, maxDual);  \
    a = vget_lane_u32(maxOfMaxDual, 0);                     \
   }

#endif

/**
 * @brief Reduces MAX of all integer in vector register into a single float
 * @param a output of 32bit integer
 * @param b input register vector
 */
#ifdef NSIMD
# define SIMD_REDUCE_MAX_EPU32(a, b) a=b
# define SIMD_REDUCE_MAX_EPU64(a, b) a=b
#elif __AVX__
# define SIMD_REDUCE_MAX_EPU32(a, b)                \
    {                                               \
    __m128i imm0;                                   \
    __SIMD_REDUCE_MAX_256_TO_128_EPU32(imm0, b);    \
    __SIMD_REDUCE_MAX_128_TO_32_EPU32(a, imm0);     \
    }
# define SIMD_REDUCE_MAX_EPU64(a, b)                \
    __SIMD_REDUCE_MAX_256_TO_64_EPU64(a,b)
#elif __SSE__
# define SIMD_REDUCE_MAX_EPU32(a, b)                \
    __SIMD_REDUCE_MAX_128_TO_32_EPU32(a, b);
# define SIMD_REDUCE_MAX_EPU64(a, b)                \
    __SIMD_REDUCE_MAX_128_TO_64_EPU64(a,b)
#elif __ARM_NEON
# define SIMD_REDUCE_MAX_EPU32(a, b)                \
    __SIMD_REDUCE_MAX_128_TO_32_EPU32_ARM(a,b)
#endif

/**
 * @brief Populate the EPU_REF "name" with mask for the lower "batch_size"
 * uint32 elements.
 */
#ifdef NSIMD
#define __SIMD_GENERATE_MASK_EPU32_0 0
#define __SIMD_GENERATE_MASK_EPU32_1 -1
#define SIMD_GENERATE_MASK_EPU32(name,batch_size)       \
    switch(batch_size) {                                \
    case 1: name = __SIMD_GENERATE_MASK_EPU32_1; break; \
    default:name = __SIMD_GENERATE_MASK_EPU32_0; break;\
    }
#define SIMD_GENERATE_MASK_EPU64(name,batch_size)       \
        SIMD_GENERATE_MASK_EPU32(name,batch_size)
#elif __AVX__
#define __SIMD_GENERATE_MASK_EPU32_0 _mm256_setzero_si256()
#define __SIMD_GENERATE_MASK_EPU32_1 _mm256_set_epi32(-0,-0,-0,-0,-0,-0,-0,-1)
#define __SIMD_GENERATE_MASK_EPU32_2 _mm256_set_epi32(-0,-0,-0,-0,-0,-0,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_3 _mm256_set_epi32(-0,-0,-0,-0,-0,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_4 _mm256_set_epi32(-0,-0,-0,-0,-1,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_5 _mm256_set_epi32(-0,-0,-0,-1,-1,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_6 _mm256_set_epi32(-0,-0,-1,-1,-1,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_7 _mm256_set_epi32(-0,-1,-1,-1,-1,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_8 _mm256_set_epi32(-1,-1,-1,-1,-1,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU64_0 _mm256_setzero_si256()
#define __SIMD_GENERATE_MASK_EPU64_1 _mm256_set_epi64x(-0,-0,-0,-1)
#define __SIMD_GENERATE_MASK_EPU64_2 _mm256_set_epi64x(-0,-0,-1,-1)
#define __SIMD_GENERATE_MASK_EPU64_3 _mm256_set_epi64x(-0,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU64_4 _mm256_set_epi64x(-1,-1,-1,-1)
#define SIMD_GENERATE_MASK_EPU32(name,batch_size)      \
    switch(batch_size) {                               \
    case 1: name = __SIMD_GENERATE_MASK_EPU32_1; break;\
    case 2: name = __SIMD_GENERATE_MASK_EPU32_2; break;\
    case 3: name = __SIMD_GENERATE_MASK_EPU32_3; break;\
    case 4: name = __SIMD_GENERATE_MASK_EPU32_4; break;\
    case 5: name = __SIMD_GENERATE_MASK_EPU32_5; break;\
    case 6: name = __SIMD_GENERATE_MASK_EPU32_6; break;\
    case 7: name = __SIMD_GENERATE_MASK_EPU32_7; break;\
    case 8: name = __SIMD_GENERATE_MASK_EPU32_8; break;\
    default:name = __SIMD_GENERATE_MASK_EPU32_0; break;\
    }

#define SIMD_GENERATE_MASK_EPU64(name,batch_size)      \
    switch(batch_size) {                               \
    case 1: name = __SIMD_GENERATE_MASK_EPU64_1; break;\
    case 2: name = __SIMD_GENERATE_MASK_EPU64_2; break;\
    case 3: name = __SIMD_GENERATE_MASK_EPU64_3; break;\
    case 4: name = __SIMD_GENERATE_MASK_EPU64_4; break;\
    default:name = __SIMD_GENERATE_MASK_EPU32_0; break;\
    }
#elif __SSE__
#define __SIMD_GENERATE_MASK_EPU32_0 _mm_setzero_si128()
#define __SIMD_GENERATE_MASK_EPU32_1 _mm_set_epi32(-0,-0,-0,-1)
#define __SIMD_GENERATE_MASK_EPU32_2 _mm_set_epi32(-0,-0,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_3 _mm_set_epi32(-0,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU32_4 _mm_set_epi32(-1,-1,-1,-1)
#define __SIMD_GENERATE_MASK_EPU64_0 _mm_setzero_si128()
#define __SIMD_GENERATE_MASK_EPU64_1 _mm_set_epi64x (-0,-1)
#define __SIMD_GENERATE_MASK_EPU64_2 _mm_set_epi64x (-1,-1)
#define SIMD_GENERATE_MASK_EPU32(name,batch_size)      \
    switch(batch_size) {                               \
    case 1: name = __SIMD_GENERATE_MASK_EPU32_1; break;\
    case 2: name = __SIMD_GENERATE_MASK_EPU32_2; break;\
    case 3: name = __SIMD_GENERATE_MASK_EPU32_3; break;\
    case 4: name = __SIMD_GENERATE_MASK_EPU32_4; break;\
    default:name = __SIMD_GENERATE_MASK_EPU32_0; break;\
    }

#define SIMD_GENERATE_MASK_EPU64(name,batch_size)      \
    switch(batch_size) {                               \
    case 1: name = __SIMD_GENERATE_MASK_EPU64_1; break;\
    case 2: name = __SIMD_GENERATE_MASK_EPU64_2; break;\
    default:name = __SIMD_GENERATE_MASK_EPU32_0; break;\
    }
#endif

/**
 * @brief For each "TARGET" unsigned integer within "VECTOR"
 * @param VECTOR an integer vector
 * @param TARGET destination variable to store results (previously declared)
 */
#ifdef NSIMD
# define SIMD_FOREACH_EPU32 (VECTOR, TARGET)                                   \
    for (TRAGET=VECTOR; TARGET==VECTOR; TARGET++)
#elif __AVX__
# define SIMD_FOREACH_EPU32(VECTOR, TARGET)                                    \
    for(CACHE_ALIGNED unsigned buf[8], *cur=buf, valid=1; valid; valid=0)      \
    for(_mm256_store_si256((__m256i *)buf, VECTOR), TARGET=*(unsigned*)cur;    \
        cur<buf+8; cur++, TARGET=*(unsigned*)cur)
#elif __SSE__
# define SIMD_FOREACH_EPU32(VECTOR, TARGET)                                    \
    for(TARGET=_mm_extract_epi32(VECTOR, 0), i=0; i<4;                         \
    i++, TARGET=_mm_extract_epi32(VECTOR, i))
#else
# error("SIMD_FOR_EACH_EPU32 not implemented for target.")
#endif

/**
 * @brief Creates a string representation of a 32bit vector
 * @param TARGET The destination buffer.
 * @param V The vector register variable name
 */
#ifdef NSIMD
# define SIMD_VECTOR32_TO_STRING(TARGET, V) \
    snprintf(TARGET, 20, "[%f]", v)
#elif __AVX512F__
# define SIMD_VECTOR32_TO_STRING(TARGET, V)                           \
    {                                                                 \
    char __simd_string[512];                                          \
    simd_element_t __simd_element;                                    \
    snprintf(TARGET, 20, "[");                                        \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,0),0); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,0),1); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,0),2); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,0),3); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,1),0); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,1),1); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,1),2); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,1),3); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,2),0); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,2),1); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,2),2); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,2),3); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,3),0); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,3),1); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,3),2); \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm512_extractf32x4_ps(V,3),3); \
    snprintf(__simd_string, 20, "%f]", __simd_element.f);             \
    strcat(TARGET, __simd_string);                                    \
    }
#elif __AVX__
# define SIMD_VECTOR32_TO_STRING(TARGET, V)                           \
    {                                                                 \
    char __simd_string[128];                                          \
    simd_element_t __simd_element;                                    \
    snprintf(TARGET, 20, "[");                                        \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,0),0);  \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,0),1);  \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,0),2);  \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,0),3);  \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,1),0);  \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,1),1);  \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,1),2);  \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(_mm256_extractf128_ps(V,1),3);  \
    snprintf(__simd_string, 20, "%f]", __simd_element.f);             \
    strcat(TARGET, __simd_string);                                    \
    }
#elif __SSE__
# define SIMD_VECTOR32_TO_STRING(TARGET, V)                           \
    {                                                                 \
    char __simd_string[64];                                           \
    simd_element_t __simd_element;                                    \
    snprintf(TARGET, 20, "[");                                        \
    __simd_element.d = _mm_extract_ps(V,0);                           \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(V,1);                           \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(V,2);                           \
    snprintf(__simd_string, 20, "%f, ", __simd_element.f);            \
    strcat(TARGET, __simd_string);                                    \
    __simd_element.d = _mm_extract_ps(V,3);                           \
    snprintf(__simd_string, 20, "%f]", __simd_element.f);             \
    strcat(TARGET, __simd_string);                                    \
    }
#elif __ARM_NEON
# define SIMD_VECTOR32_TO_STRING(TARGET, V)                           \
    {                                                                 \
        char __simd_string[128];                                      \
        float32x2_t lodual = vget_low_f32(V);                         \
        float32x2_t hidual = vget_high_f32(V);                        \
        snprintf(__simd_string, 20, "[%f, ",vget_lane_f32(lodual,0)); \
        strcat(TARGET, __simd_string);                                \
        snprintf(__simd_string, 20, "%f, ", vget_lane_f32(lodual,1)); \
        strcat(TARGET, __simd_string);                                \
        snprintf(__simd_string, 20, "%f, ", vget_lane_f32(hidual,0)); \
        strcat(TARGET, __simd_string);                                \
        snprintf(__simd_string, 20, "%f]", vget_lane_f32(hidual,1));  \
        strcat(TARGET, __simd_string);                                \
    }
#endif


#endif /* SIMD_COMMON_H */
