#pragma once
#include <type_traits>

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

