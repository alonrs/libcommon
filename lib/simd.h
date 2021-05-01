#ifndef _SIMD_H
#define _SIMD_H

#include <stdio.h>
#include "util.h"

#ifdef __SSE__
 #include <x86intrin.h>
#endif

/**
 * @brief General defines that change according to available SIMD engine
 */
#ifdef NSIMD
# define SIMD_WIDTH 1
# define PS_REG float
# define EPU_REG unsigned int
#elif __AVX512F__
# define SIMD_WIDTH 16
# define PS_REG __m512
# define EPU_REG __m512i
# define SIMD_COMMAND(X) _mm512 ## X
#elif __AVX__
# define SIMD_WIDTH 8
# define PS_REG __m256
# define EPU_REG __m256i
# define SIMD_COMMAND(X) _mm256 ## X
#elif __SSE__
# define SIMD_WIDTH 4
# define PS_REG __m128
# define EPU_REG __m128i
# define SIMD_COMMAND(X) _mm ## X
#endif

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
#elif __AVX512F__
# define SIMD_FMA_PS(a,b,c) \
  a = _mm512_fmadd_ps(a, b, c)
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
#endif

/**
 * @brief For clean code of load_ps.
 * @param a memory location, must be aligned
 * (64bit/32bit/16bit, depends on vector width)
 */
#ifdef NSIMD
# define SIMD_LOAD_PS(a) *a
# define SIMD_LOADU_PS(a) *a
# define SIMD_LOADU_SI(a) *a
#else
# define SIMD_LOAD_PS(a) SIMD_COMMAND(_load_ps(a))
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
#else
# define SIMD_STORE_PS(a,b) SIMD_COMMAND(_store_ps(a,b))
# define SIMD_STORE_PS(a,b) SIMD_COMMAND(_store_ps(a,b))
#endif

/**
 * @brief For clean code of set1.
 * @param a float value
 */
#ifdef NSIMD
# define SIMD_SET1_PS(a) (float)a
# define SIMD_SET1_EPI(a) (int)a
#else
# define SIMD_SET1_PS(a) SIMD_COMMAND(_set1_ps(a))
# define SIMD_SET1_EPI(a) SIMD_COMMAND(_set1_epi32(a))
#endif

/**
 * @brief For clean code of zeros vector.
 */
#ifdef NSIMD
# define SIMD_ZEROS_PS 0
#else
# define SIMD_ZEROS_PS SIMD_COMMAND(_setzero_ps())
#endif

/**
 * @brief For clean code of subtract.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_SUB_PS(a,b) a-b
#else
# define SIMD_SUB_PS(a,b) SIMD_COMMAND(_sub_ps(a, b))
#endif

/**
 * @brief For clean code of addition.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_ADD_PS(a,b) a+b
#else
# define SIMD_ADD_PS(a,b) SIMD_COMMAND(_add_ps(a, b))
#endif

/**
 * @brief For clean code of divide.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_DIV_PS(a,b) a/b
#else
# define SIMD_DIV_PS(a,b) SIMD_COMMAND(_div_ps(a, b))
#endif

/**
 * @brief For clean code of multiply.
 * @param a vector register
 * @param b vector register
 */
#ifdef NSIMD
# define SIMD_MUL_PS(a,b) a*b
#else
# define SIMD_MUL_PS(a,b) SIMD_COMMAND(_mul_ps(a, b))
#endif

/**
 * @brief For clean code of and.
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
# define SIMD_AND_PS(a,b) simd_helper_and_ps__(a, b)
#else
# define SIMD_AND_PS(a,b) SIMD_COMMAND(_and_ps(a, b))
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
#elif __AVX512F__
# define SIMD_MAX_EPU32(a,b,c) a=_mm512_max_epu32(b,c)
# define SIMD_MIN_EPU32(a,b,c) a=_mm512_min_epu32(b,c)
#elif __AVX2__
# define SIMD_MAX_EPU32(a,b,c) a=_mm256_max_epu32(b,c)
# define SIMD_MIN_EPU32(a,b,c) a=_mm256_min_epu32(b,c)
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
#elif __SSE__
# define SIMD_MAX_EPU32(a,b,c) a=_mm_min_epu32(b,c)
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
#endif

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
 * @brief Reduces sum of 512bit vector of floats to 256bit vector of floats
 * @param a output 256bit float
 * @param b input 512bit float
 */
#define __SIMD_REDUCE_SUM_512_TO_256_PS(a, b)     \
    {                                             \
    __m256 hiOcta = _mm512_extractf32x8_ps(b, 1); \
    __m256 loOcta = _mm512_castps512_ps256(b);    \
    a = _mm256_add_ps(loOcta, hiOcta);            \
    }

/**
 * @brief Reduces SUM of all floats in vector register into a single float
 * @param a output of 32bit float
 * @param b input register vector
 */
#ifdef NSIMD
# define SIMD_REDUCE_SUM_PS(a, b) a=b
#elif __AVX512F__
# define SIMD_REDUCE_SUM_PS(a, b)                \
    {                                            \
    __m256 imm0;                                 \
    __m128 imm1;                                 \
    __SIMD_REDUCE_SUM_512_TO_256_PS(imm0, b);    \
    __SIMD_REDUCE_SUM_256_TO_128_PS(imm1, imm0); \
    __SIMD_REDUCE_SUM_128_TO_32_PS(a, imm1);     \
    }
#elif __AVX__
# define SIMD_REDUCE_SUM_PS(a, b)                \
    {                                            \
    __m128 imm0;                                 \
    __SIMD_REDUCE_SUM_256_TO_128_PS(imm0, b);    \
    __SIMD_REDUCE_SUM_128_TO_32_PS(a, imm0);     \
    }
#elif __SSE__
# define SIMD_REDUCE_SUM_PS(a, b) \
    __SIMD_REDUCE_SUM_128_TO_32_PS(a, b);
#endif

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


/**
 * @brief Reduces MAX of all floats in vector register into a single float
 * @param a output of 32bit float
 * @param b input register vector
 */
#ifdef NSIMD
# define SIMD_REDUCE_MAX_PS(a, b) std::max(a,b)
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
# define SIMD_REDUCE_MAX_PS(a, b) \
    __SIMD_REDUCE_MAX_128_TO_32_PS(a, b);
#endif

/**
 * @brief Reduces max of 128bit vector of uint32 to uint32
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

/**
 * @brief Reduces max of 512bit vector of floats to 256bit vector of floats
 * @param a output 256bit float
 * @param b input 512bit float
 */
#define __SIMD_REDUCE_MAX_512_TO_256_EPU32(a, b)      \
    {                                                 \
    __m256i hiOcta = _mm512_extracti32x8_epi32(b, 1); \
    __m256i loOcta = _mm512_castsi512_si256(b);       \
    a = _mm256_max_epu32(loOcta, hiOcta);             \
    }

/**
 * @brief Reduces MAX of all floats in vector register into a single float
 * @param a output of 32bit float
 * @param b input register vector
 */
#ifdef NSIMD
# define SIMD_REDUCE_MAX_EPU32(a, b) std::max(a,b)
#elif __AVX512F__
# define SIMD_REDUCE_MAX_EPU32(a, b)                \
    {                                               \
    __m256i imm0;                                   \
    __m128i imm1;                                   \
    __SIMD_REDUCE_MAX_512_TO_256_EPU32(imm0, b);    \
    __SIMD_REDUCE_MAX_256_TO_128_EPU32(imm1, imm0); \
    __SIMD_REDUCE_MAX_128_TO_32_EPU32(a, imm1);     \
    }
    #elif __AVX2__ || __AVX__
# define SIMD_REDUCE_MAX_EPU32(a, b)                \
    {                                               \
    __m128i imm0;                                   \
    __SIMD_REDUCE_MAX_256_TO_128_EPU32(imm0, b);    \
    __SIMD_REDUCE_MAX_128_TO_32_EPU32(a, imm0);     \
    }
#elif __SSE__
# define SIMD_REDUCE_MAX_EPU32(a, b)                    \
    __SIMD_REDUCE_MAX_128_TO_32_EPU32(a, b);
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
#endif


#endif /* SIMD_COMMON_H */
