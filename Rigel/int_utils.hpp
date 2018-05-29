#pragma once

#ifdef __GNUC__
#define intel_intrinsic_uint64  long long unsigned int
#define fixed_subborrow_u64(borrow, a, b, result) _subborrow_u64(borrow, b, a, result)
#else
#define intel_intrinsic_uint64  unsigned __int64
#define fixed_subborrow_u64(borrow, a, b, result) _subborrow_u64(borrow, a, b, result)
#endif

