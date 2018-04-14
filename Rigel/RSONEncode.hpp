
#include <cstdint>
#include <ostream>
#include <vector>
#include <map>
#include <limits>
#include <string>
#include <boost/none.hpp>

#include "RSON.hpp"
#include "RSONLength.hpp"

namespace Orion {
namespace Rigel {
namespace RSON {

static inline void encode(std::string &s, bool value)
{
    s += value ? TRUE_CODE : FALSE_CODE;
}

static inline void encode(std::string &s, boost::none_t value)
{
    s += NONE_CODE;
}

template<typename T>
static inline void encodeInteger(std::string &s, T value)
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

    s += (more ? 0x80 : 0xc0) | part;

    // following parts are 7 bits + stop-bit
    while (more) {
        part = value & 0x7f;
        top_bit = part >> 6;
        value >>= 7;
        more = value != end_value || top_bit != end_top_bit;

        s += (more ? 0x00 : 0x80) | part;
    }
}

static inline void encode(std::string &s, __int128_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, int64_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, int32_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, int16_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, int8_t value) { encodeInteger(s, value); }

static inline void encode(std::string &s, __uint128_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, uint64_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, uint32_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, uint16_t value) { encodeInteger(s, value); }
static inline void encode(std::string &s, uint8_t value) { encodeInteger(s, value); }


template<typename T>
static inline void splitFloatingPoint(T value, int32_t &exponent, int64_t &mantissa)
{
    static_assert(std::numeric_limits<T>::is_iec559, "Can only encode IEEE754 floating point");
    static_assert(std::numeric_limits<T>::radix == 2, "Can only encode IEEE754 floating point");

    const int MANTISSA_WIDTH = std::numeric_limits<T>::digits - 1;
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
    } else if (sizeof (value) == 8) {
        uint64_t tmp;
        memcpy(&tmp, &value, sizeof (tmp));
        value_as_int = tmp;
    } else {
        throw std::domain_error("Can not encode float of this number of bytes.");
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
        throw std::domain_error("Can not encode signalling NaN");

    } else if (exponent == EXPONENT_INF) { // Quiet Nan
        exponent = INT32_MIN;

    } else { // Normal
        exponent = exponent - EXPONENT_BIAS - MANTISSA_WIDTH;
        mantissa |= MANTISSA_IMPLICIT_BIT;
    }
}

template<typename T>
static inline void encodeFloat(std::string &s, T value)
{
    int32_t exponent;
    int64_t mantissa;

    splitFloatingPoint(value, exponent, mantissa);

    s += BINARY_FLOAT_CODE;

    switch (exponent) {
    case INT32_MIN: // NaN
        encode(s, mantissa);
        encode(s, boost::none);
        break;

    case INT32_MAX: // Infinite
        encode(s, static_cast<bool>(mantissa == INT64_MIN));
        break;

    default: // Number
        if (mantissa == 0) {
            encode(s, 0);

        } else {
            // Normalize by dropping trailing zero bits.
            while ((mantissa & 1) == 0) {
                mantissa >>= 1;
                exponent++;
            }
            encode(s, mantissa);
            encode(s, exponent);
        }
    }
}

static inline void encode(std::string &s, float value) { encodeFloat(s, value); }
static inline void encode(std::string &s, double value) { encodeFloat(s, value); }

static inline void encode(std::string &s, const std::string &value)
{
    auto start_position = s.length();
    bool is_ascii = true;

    // Start copying the string-value into the output.
    // Adjust when the string-value is not ASCII.
    for (const auto &c : value) {
        if (c == 0x00) {
            throw std::out_of_range("A UTF-8 encoded string may not contain a NUL character");
        }

        s += c;
        if (is_ascii && (c < 0 || (c >= 0x10 && c <= 0x1a))) {
            // Since this is not an ASCII string, we insert an UTF8_STRING_CODE.
            // Which will cause the string to do a memcpy() operation. So start
            // this early.
            s.insert(start_position, 1, UTF8_STRING_CODE);
            is_ascii = false;
        }
    }

    // An ASCII string must have at least two characters.
    // Because the first character has the stop-bit unset.
    // It can also not have a non-ASCII character or a
    // control-character between 0x10 and 0x1a.
    if (is_ascii && (value.length() < 2)) {
        s.insert(start_position, 1, UTF8_STRING_CODE);
        is_ascii = false;
    }

    if (is_ascii) {
        // Set the stop-bit on the last character of the ASCII-string.
        s.back() |= 0x80;
    } else {
        // Terminate the utf-8 string with nul.
        s += MARK_CODE;
    }
}

template<typename T>
static inline void encodeByteArray(std::string &s, const std::basic_string<uint8_t> &value)
{
    s += BYTE_ARRAY_CODE;

    size_t nr_full_chunks = value.length() / 255;
    size_t last_chunk_size = value.length() % 255;

    // Copy chunks of 255 bytes length.
    for (off_t chunk = 0; chunk < nr_full_chunks; chunk++) {
        s += static_cast<char>(0xff);
        s.append(reinterpret_cast<char *>(&value.data()[chunk * 255]), 255);
    }

    // Last chunk, with length less than 255, including zero.
    s += static_cast<char>(last_chunk_size);
    s.append(reinterpret_cast<char *>(&value.data()[value.length() - last_chunk_size]), last_chunk_size);
}

// Vector-encode requires a prototype of map-encode so it can encode a vector of maps.
template<typename U, typename V>
static inline void encode(std::string &s, const std::map<U, V> &container);

template<typename T>
static inline void encode(std::string &s, const std::vector<T> &container)
{
    s += LIST_CODE;

    for (auto const &item: container) {
        encode(s, item);
    }

    s += MARK_CODE;
}

template<typename U, typename V>
static inline void encode(std::string &s, const std::map<U, V> &container)
{
    s += OBJECT_CODE;

    // Maps are already sorted by key.
    for (auto &item: container) {
        encode(s, item.first);
    }

    s += MARK_CODE;

    for (auto &item: container) {
        encode(s, item.second);
    }

    s += MARK_CODE;
}

template<typename T>
static inline std::string encode(const T &value)
{
    std::string s;

    s.reserve(length(value));
    encode(s, value);

    return s;
}

template<typename T>
static inline void encode(std::ostream &stream, const T &value)
{
    auto s = encode(value);
    stream.write(s.data(), s.length());
}

};};};