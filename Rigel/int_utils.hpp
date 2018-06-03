#pragma once
#include <type_traits>

#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>

#ifdef __GNUC__
#define intel_intrinsic_uint64  long long unsigned int
#define fixed_subborrow_u64(borrow, a, b, result) _subborrow_u64(borrow, b, a, result)
#else
#define intel_intrinsic_uint64  unsigned __int64
#define fixed_subborrow_u64(borrow, a, b, result) _subborrow_u64(borrow, a, b, result)
#endif

template<typename R, typename T>
static inline bool isAligned(T *p)
{
    auto _p = reinterpret_cast<uintptr_t>(p);

    return (_p % std::alignment_of<R>::value) == 0;
}

/** Return the number of items in size.
 * The potentially partial item at the end of size will be
 * included in the result.
 */
template<typename R>
static inline size_t nrItems(size_t size)
{
    return (size + sizeof(R) - 1) / sizeof(R);
}

/** Extend size so it will include the partial item at the end.
 */
template<typename R>
static inline size_t extendSize(size_t size)
{
    return nrItems<R>(size) * sizeof(R);
}

static inline uint64_t rotr(uint64_t x, int shift)
{
    return (x >> shift) | (x << (64 - shift));
}

static inline __m128i mm_inc_si128(__m128i counter)
{
    // Full 128-bit increment-by-one.
    // compare =   hi, lo     hi, ff
    // check   =   hi, ff     hi, ff
    // cmpeq   =   ff, 00     ff, ff
    // one     =   00, ff     ff, ff
    auto check = _mm_insert_epi64(counter, 0xffffffffffffffff, 0);
    auto reverse_one = _mm_cmpeq_epi64(counter, check);
    auto one = _mm_shuffle_epi32(reverse_one, _MM_SHUFFLE(1, 0, 3, 2));
    return _mm_sub_epi64(counter, one);
}

