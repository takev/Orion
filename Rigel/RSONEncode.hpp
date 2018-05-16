
#include <cstdint>
#include <ostream>
#include <vector>
#include <map>
#include <limits>
#include <string>
#include <type_traits>
#include <boost/none.hpp>
#include <boost/exception/all.hpp>

#include "RSON.hpp"
#include "RSONLength.hpp"

namespace Orion {
namespace Rigel {
namespace RSON {

struct encode_error: virtual boost::exception {};
struct encode_type_error: virtual encode_error, virtual std::exception {};
struct encode_value_error: virtual encode_error, virtual std::exception {};

static inline void encode(std::string &s, bool value)
{
    s += value ? TRUE_CODE : FALSE_CODE;
}

static inline void encode(std::string &s, boost::none_t value)
{
    s += NONE_CODE;
}

template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
static inline void encode(std::string &s, T value)
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
        BOOST_THROW_EXCEPTION(encode_type_error());
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
        BOOST_THROW_EXCEPTION(encode_value_error());

    } else if (exponent == EXPONENT_INF) { // Quiet Nan
        exponent = INT32_MIN;

    } else { // Normal
        exponent = exponent - EXPONENT_BIAS - MANTISSA_WIDTH;
        mantissa |= MANTISSA_IMPLICIT_BIT;
    }
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
static inline void encode(std::string &s, T value)
{
    int32_t exponent;
    int64_t mantissa;

    splitFloatingPoint(value, exponent, mantissa);

    s += BINARY_FLOAT_CODE;

    switch (exponent) {
    case INT32_MIN: // NaN
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

static inline void encode(std::string &s, const std::string &value)
{
    auto start_position = s.length();
    bool is_ascii = true;

    // Start copying the string-value into the output.
    // Adjust when the string-value is not ASCII.
    for (const auto &c : value) {
        if (c == 0x00) {
            BOOST_THROW_EXCEPTION(encode_value_error());
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

    if (is_ascii) {
        switch (value.length()) {
        case 0:
            // A zero length ASCII string must be encoded as
            // a UTF-8 string. Which will cause two bytes to
            // be emited in the stream.
            s.insert(start_position, 1, UTF8_STRING_CODE);
            is_ascii = false;
            break;

        case 1:
            // A one length ASCII string must be terminated with a nul, with
            // the stop bit set. Which will cause two bytes to
            // be emited in the stream.
            s += MARK_CODE;
            break;

        default:
            break;
        }
    }

    if (is_ascii) {
        // Set the stop-bit on the last character of the ASCII-string.
        s.back() |= 0x80;
    } else {
        // Terminate the utf-8 string with nul.
        s += MARK_CODE;
    }
}

static inline void encode(std::string &s, const std::basic_string<uint8_t> &value)
{
    s += BYTE_ARRAY_CODE;

    size_t nr_full_chunks = value.length() / 255;
    size_t last_chunk_size = value.length() % 255;

    // Copy chunks of 255 bytes length.
    for (size_t chunk = 0; chunk < nr_full_chunks; chunk++) {
        s += static_cast<char>(0xff);
        s.append(reinterpret_cast<const char *>(&value.data()[chunk * 255]), 255);
    }

    // Last chunk, with length less than 255, including zero.
    s += static_cast<char>(last_chunk_size);
    s.append(reinterpret_cast<const char *>(&value.data()[value.length() - last_chunk_size]), last_chunk_size);
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
    s += DICTIONARY_CODE;

    // Maps are already sorted by key.
    for (auto &item: container) {
        encode(s, item.first);
    }

    s += MARK_CODE;

    for (auto &item: container) {
        encode(s, item.second);
    }
}

/** Encode a C++ value into RSON.
 * This function will reserve space in the string for the whole value to be
 * encoded. This should improve encoding performance by elmintating multiple
 * memory allocations during appending to the string.
 *
 * The returned string may contain nul-characters.
 *
 * @param value The value to be encoded to RSON.
 * @return The RSON encoded value as a string.
 */
template<typename T>
static inline std::string encode(const T &value)
{
    std::string s;

    s.reserve(length(value));
    encode(s, value);

    return s;
}

/** Encode a C++ value into RSON and send it to a stream.
 *
 * @param stream The stream to write the RSON encoded value to.
 * @param value The value to be encoded to RSON.
 */
template<typename T>
static inline void encode(std::ostream &stream, const T &value)
{
    auto s = encode(value);
    stream.write(s.data(), s.length());
}

};};};
