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
static inline bool isAlignedTo(T *p)
{
    auto _p = reinterpret_cast<uintptr_t>(p);

    return (_p % std::alignment_of<R>::value) == 0;
}

