
#include <cstdint>
#include <ostream>
#include <vector>
#include <map>
#include <boost/none.hpp>

namespace Orion {
namespace Rigel {
namespace CBSON {

using namespace std;
using namespace boost;

const char MARK_CODE = 0x00;
const char NONE_CODE = 0x10;
const char TRUE_CODE = 0x11;
const char FALSE_CODE = 0x12;
const char LIST_CODE = 0x13;
const char OBJECT_CODE = 0x14;
const char DECIMAL_FLOAT_CODE = 0x15;
const char BINARY_FLOAT_CODE = 0x16;
const char BYTE_ARRAY_CODE = 0x17;
const char UTF8_STRING_CODE = 0x18;
const char RESERVED1_CODE = 0x19;
const char RESERVED2_CODE = 0x1a;

static inline void encode(ostream &stream, bool value)
{
    stream.put(value ? TRUE_CODE : FALSE_CODE);
}

static inline void encode(ostream &stream, none_t value)
{
    stream.put(NONE_CODE);
}

template<typename T>
static inline void encodeInteger(ostream &stream, T value)
{
    T end_value = value >= 0 ? 0 : -1;
    uint8_t end_top_bit = value >= 0 ? 0 : 1;
    uint8_t part;
    uint8_t top_bit;
    bool more;

    // First part is 6 bits + stop-bit
    part = value & 0x3f;
    top_bit = part >> 5;
    value >>= 6;
    more = value != end_value || top_bit != end_top_bit;

    stream.put((more ? 0x80 : 0xc0) | part);

    // following parts are 7 bits + stop-bit
    while (more) {
        part = value & 0x7f;
        top_bit = part >> 6;
        value >>= 7;
        more = value != end_value || top_bit != end_top_bit;

        stream.put((more ? 0x00 : 0x80) | part);
    }
}

static inline void encode(ostream &stream, __int128_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, int64_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, int32_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, int16_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, int8_t value) { encodeInteger(stream, value); }

static inline void encode(ostream &stream, __uint128_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, uint64_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, uint32_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, uint16_t value) { encodeInteger(stream, value); }
static inline void encode(ostream &stream, uint8_t value) { encodeInteger(stream, value); }


template<typename T>
static inline void splitFloatingPoint(T value, int32_t &exponent, int64_t &mantissa)
{
    static_assert(sizeof(value) == 4 || sizeof(value) == 8, "Only binary32 and binary64 are supported.");
    static_assert(is_floating_point<T>::value, "Can only convert floating point numbers.");

    const int MANTISSA_WIDTH = sizeof(value) == 4 ? 23 : 52;
    const int EXPONENT_WIDTH = sizeof(value) == 4 ? 8 : 11;

    const uint64_t MANTISSA_MASK = (1ULL << MANTISSA_WIDTH) - 1;
    const uint64_t EXPONENT_MASK = (1ULL << EXPONENT_WIDTH) - 1;

    const int64_t MANTISSA_INF = 0;
    const int64_t MANTISSA_IMPLICIT_BIT = 1LL << MANTISSA_WIDTH;
    const int64_t MANTISSA_SIGNALLING_NAN = 1LL << (MANTISSA_WIDTH - 1);
    const int32_t EXPONENT_BIAS = (1L << (EXPONENT_WIDTH - 1)) - 1;
    const int32_t EXPONENT_INF = EXPONENT_MASK;
    const int32_t EXPONENT_DENORMAL = 0;

    uint64_t value_as_int;
    if (sizeof (value) == 4) {
        uint32_t tmp;
        memcpy(&tmp, &value, sizeof (tmp));
        value_as_int = tmp;
    } else {
        uint64_t tmp;
        memcpy(&tmp, &value, sizeof (tmp));
        value_as_int = tmp;
    }

    bool sign = (value_as_int >> (MANTISSA_WIDTH + EXPONENT_WIDTH)) > 0;
    exponent = (value_as_int >> MANTISSA_WIDTH) & EXPONENT_MASK;
    mantissa = value_as_int & MANTISSA_MASK;

    if (exponent == EXPONENT_DENORMAL) { // Denormal
        exponent = EXPONENT_BIAS - MANTISSA_WIDTH - 1;

    } else if (exponent == EXPONENT_INF && mantissa == MANTISSA_INF) { // Infinite
        exponent = INT32_MAX;
        mantissa = sign ? INT64_MIN : INT64_MAX;

    } else if (exponent == EXPONENT_INF && (mantissa & MANTISSA_SIGNALLING_NAN) > 0) { // Signalling Nan
        throw domain_error("Can not encode signalling NaN");

    } else if (exponent == EXPONENT_INF) { // Quiet Nan
        exponent = INT32_MIN;

    } else { // Normal
        exponent = exponent - EXPONENT_BIAS - MANTISSA_WIDTH;
        mantissa |= MANTISSA_IMPLICIT_BIT;
    }
}

template<typename T>
static inline void encodeFloat(ostream &stream, T value)
{
    int32_t exponent;
    int64_t mantissa;

    splitFloatingPoint(value, exponent, mantissa);

    cerr << "exponent=" << exponent << ", mantissa=" << mantissa << endl;

    stream.put(BINARY_FLOAT_CODE);

    switch (exponent) {
    case INT32_MIN: // NaN
        encode(stream, mantissa);
        encode(stream, none);
        break;

    case INT32_MAX: // Infinite
        encode(stream, static_cast<bool>(mantissa == INT64_MIN));
        break;

    default: // Number
        if (mantissa == 0) {
            encode(stream, 0);

        } else {
            // Normalize by dropping trailing zero bits.
            while ((mantissa & 1) == 0) {
                mantissa >>= 1;
                exponent++;
            }
            encode(stream, mantissa);
            encode(stream, exponent);
        }
    }
}

static inline void encode(ostream &stream, float value) { encodeFloat(stream, value); }
static inline void encode(ostream &stream, double value) { encodeFloat(stream, value); }

static inline void encode(ostream &stream, const string &value)
{
    // An ASCII string must have at least two characters.
    // Because the first character has the stop-bit unset.
    // It can also not have a non-ASCII character or a
    // control-character between 0x10 and 0x1a.
    if (value.length() < 2) {
        goto utf8_string_encoding;
    }

    for (const auto &c : value) {
        if (c == 0x00) {
            throw out_of_range("A UTF-8 encoded string may not contain a NUL character");
        }

        if (c < 0 || (c >= 0x10 && c <= 0x1a)) {
            goto utf8_string_encoding;
        }
    }

    stream.write(value.data(), value.length() - 1);
    stream.put(value[value.length() - 1] | 0x80);
    return;

utf8_string_encoding:
    stream.put(UTF8_STRING_CODE);
    stream.write(value.data(), value.length());
    stream.put(MARK_CODE);
}

template<typename T>
static inline void encodeByteArray(ostream &stream, const basic_string<uint8_t> &value)
{
    stream.put(BYTE_ARRAY_CODE);

    size_t nr_full_chunks = value.length() / 255;
    size_t last_chunk_size = value.length() % 255;

    // Copy chunks of 255 bytes length.
    for (off_t chunk = 0; chunk < nr_full_chunks; chunk++) {
        stream.put(0xff);
        stream.write(reinterpret_cast<char *>(&value.data()[chunk * 255]), 255);
    }

    // Last chunk, with length less than 255, including zero.
    stream.put(last_chunk_size);
    stream.write(reinterpret_cast<char *>(&value.data()[value.length() - last_chunk_size]), last_chunk_size);
}

// Vector-encode requires a prototype of map-encode so it can encode a vector of maps.
template<typename U, typename V>
static inline void encode(ostream &stream, const map<U, V> &container);

template<typename T>
static inline void encode(ostream &stream, const vector<T> &container)
{
    stream.put(LIST_CODE);

    for (auto const &item: container) {
        encode(stream, item);
    }

    stream.put(MARK_CODE);
}

template<typename U, typename V>
static inline void encode(ostream &stream, const map<U, V> &container)
{
    stream.put(OBJECT_CODE);

    // Maps are already sorted by key.
    for (auto &item: container) {
        encode(stream, item.first);
    }

    stream.put(MARK_CODE);

    for (auto &item: container) {
        encode(stream, item.second);
    }

    stream.put(MARK_CODE);
}

};};};
