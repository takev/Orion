
#pragma once
#include <cstdint>

using binary32_t = float;
using binary64_t = double;
using binary128_t = __float128;

using uint128_t = __uint128_t;
using int128_t = __int128_t;

using u8string = basic_string<uint8_t>;

using conv128_t = union {
    uint128_t   f128;
    int128_t    i128;
    uint64_t    u64[2];
    int64_t     i64[2];
    uint32_t    u32[4];
    int32_t     i32[4];
    uint16_t    u16[8];
    int16_t     i16[8];
    uint8_t     u8[16];
    int8_t      i8[16];
};

using conv64_t = union {
    binary64_t  f64;
    uint64_t    u64;
    int64_t     i64;
    uint32_t    u32[2];
    int32_t     i32[2];
    uint16_t    u16[4];
    int16_t     i16[4];
    uint8_t     u8[8];
    int8_t      i8[8];
};

using conv32_t = union {
    binary32_t  f32;
    uint32_t    u32;
    int32_t     i32;
    uint16_t    u16[2];
    int16_t     i16[2];
    uint8_t     u8[4];
    int8_t      i8[4];
};

using conv16_t = union {
    uint16_t    u16;
    int16_t     i16;
    uint8_t     u8[2];
    int8_t      i8[2];
};

using conv8_t = union {
    uint8_t     u8;
    int8_t      i8;
};

