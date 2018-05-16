#include <cstdint>
#include <ostream>
#include <vector>
#include <map>
#include <boost/any.hpp>
#include <boost/none.hpp>
#include <boost/exception/all.hpp>

#include "RSON.hpp"

namespace Orion {
namespace Rigel {
namespace RSON {

// Forward for decoding anything in a list and dictionary.
static inline boost::any decode(std::istream &stream);

enum class Type {
    EndOfFile,
    None,
    Boolean,
    List,
    Dictionary,
    DecimalFloat,
    BinaryFloat,
    ByteArray,
    String,
    Integer
};

struct decode_error: virtual boost::exception {};
struct decode_overflow_error: virtual decode_error, virtual std::exception {};
struct decode_eof_error: virtual decode_error, virtual std::exception {};
struct decode_code_error: virtual decode_error, virtual std::exception {};
struct decode_type_error: virtual decode_error, virtual std::exception {};

typedef boost::error_info<struct tag_code,char> code_info;
typedef boost::error_info<struct tag_type,Type> type_info;

static inline Type peekType(std::istream &stream)
{
    auto p = stream.peek();

    if (p == std::char_traits<char>::eof()) {
        return Type::EndOfFile;

    } else {
        auto c = std::char_traits<char>::to_char_type(p);
        switch (c) {
        case NONE_CODE: return Type::None;
        case TRUE_CODE: return Type::Boolean;
        case FALSE_CODE: return Type::Boolean;
        case LIST_CODE: return Type::List;
        case DICTIONARY_CODE: return Type::Dictionary;
        case DECIMAL_FLOAT_CODE: return Type::DecimalFloat;
        case BINARY_FLOAT_CODE: return Type::BinaryFloat;
        case BYTE_ARRAY_CODE: return Type::ByteArray;
        case UTF8_STRING_CODE: return Type::String;
        case MARK_CODE: BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
        case RESERVED1_CODE: BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
        case RESERVED2_CODE: BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));

        default:
            if (c < 0) {
                return Type::Integer;
            } else {
                return Type::String;
            }
        }
    }
}

static inline uint8_t peekNoEOF(std::istream &stream)
{
    auto p = stream.peek();

    if (p == std::char_traits<char>::eof()) {
        BOOST_THROW_EXCEPTION(decode_eof_error());
    }

    return static_cast<uint8_t>(std::char_traits<char>::to_char_type(p));
}

static inline uint8_t getNoEOF(std::istream &stream)
{
    auto p = stream.get();

    if (p == std::char_traits<char>::eof()) {
        BOOST_THROW_EXCEPTION(decode_eof_error());
    }

    return static_cast<uint8_t>(std::char_traits<char>::to_char_type(p));
}

static inline void readIntoNoEOF(std::basic_string<uint8_t> &dst, std::istream &stream, uint8_t size)
{
    uint8_t buffer[255];

    stream.read(reinterpret_cast<char *>(buffer), size);

    if (stream.eofbit) {
        BOOST_THROW_EXCEPTION(decode_eof_error());
    }

    dst.append(buffer, size);
}

template<typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<bool, T>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    // Extract first 6 bits.
    auto c = getNoEOF(stream);
    
    if ((c & 0x80) == 0) {
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
    }

    bool stop = (c & 0x40) > 0;
    T value = c & 0x3f;

    // Extract next 7 bits one byte at a time.
    int bit_offset = 6;
    uint8_t septet;
    while (!stop) {
        auto c = getNoEOF(stream);

        stop = (c & 0x80) > 0;
        septet = c & 0x7f;
        value |= septet << bit_offset;
        bit_offset += 7;
    }

    int unused_bits = (sizeof (value) * 8) - bit_offset;
    int overflowed_bits = -unused_bits;

    if (overflowed_bits > 7) {
        BOOST_THROW_EXCEPTION(decode_overflow_error());

    } else if (overflowed_bits > 0) {
        // Sign extend the last septet, this is always signed.
        // Get just the bits that overflowed.
        int8_t last_septet = septet;
        last_septet <<= 1;
        last_septet >>= (8 - overflowed_bits);

        if (std::is_unsigned<T>::value && last_septet < 0) {
            BOOST_THROW_EXCEPTION(decode_overflow_error());
        }
        if (last_septet != 0 && last_septet != -1) {
            BOOST_THROW_EXCEPTION(decode_overflow_error());
        }
    }

    // sign extend the value. Shift left to the edge, then shift right again.
    // For unsigned types should be a no-op.
    if (unused_bits > 0) {
        value <<= unused_bits;
        value >>= unused_bits;
    }

    return value;
}

template<typename T, typename std::enable_if<std::is_same<std::string, T>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    T r;

    auto c = getNoEOF(stream);
   
    if (c == UTF8_STRING_CODE) {
        // nil terminated UTF-8 string.
        while (1) {
            c = getNoEOF(stream);

            if (c) {
                r += static_cast<char>(c);
            } else {
                return r;
            }
        }

    } else if (c >= 0x80 || (c >= 0x10 && c <= 0x1a)) {
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));

    } else {
        // stop-bit encoded ASCII string. May be terminated with a nil+stop-bit.
        bool stop = false;
        while (1) {
            if (c) {
                r += static_cast<char>(c);
            }

            if (stop) {
                return r;
            }

            c = getNoEOF(stream);
            stop = (c & 0x80) > 0;
            c &= 0x7f;
        }
    }
}

template<typename T, typename std::enable_if<std::is_same<std::basic_string<uint8_t>, T>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    T r;

    auto c = getNoEOF(stream);
   
    if (c != BYTE_ARRAY_CODE) {
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
    }

    uint8_t chunk_size;
    do {
        chunk_size = getNoEOF(stream);
        if (chunk_size > 0) {
            readIntoNoEOF(r, stream, chunk_size);
        }

    } while (chunk_size == 255);

    return r;
}

template<typename T, typename std::enable_if<std::is_same<bool, T>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    auto c = getNoEOF(stream);

    switch (c) {
    case TRUE_CODE: return true;
    case FALSE_CODE: return false;
    default:
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
    }
}

template<typename T, typename std::enable_if<std::is_base_of<boost::none_t, T>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    auto c = getNoEOF(stream);

    if (c != NONE_CODE) {
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
    }

    return boost::none;
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    auto c = getNoEOF(stream);

    if (c != BINARY_FLOAT_CODE) {
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
    }

    switch (Type t = peekType(stream)) {
    case Type::None: // NaN
        decode<boost::none_t>(stream);
        return std::numeric_limits<T>::quiet_NaN();

    case Type::Boolean: // true = -Inf, false = Inf
        if (decode<bool>(stream)) {
            return -std::numeric_limits<T>::infinity();
        } else {
            return std::numeric_limits<T>::infinity();
        }

    case Type::Integer: // mantissa
        {
            auto mantissa = decode<int64_t>(stream);
            if (mantissa == 0) {
                return static_cast<T>(0.0);

            } else {
                auto exponent = decode<int64_t>(stream);

                return static_cast<T>(static_cast<double>(mantissa) * exp2(static_cast<double>(exponent)));
            }
        }

    default:
        BOOST_THROW_EXCEPTION(decode_type_error() << type_info(t));
    }
}

template<typename T, typename std::enable_if<std::is_same<T, std::vector<boost::any>>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    auto c = getNoEOF(stream);
    if (c != LIST_CODE) {
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
    }

    auto r = T();
    while (peekNoEOF(stream) != MARK_CODE) {
        r.push_back(decode(stream));
    }

    // Read the actual mark symbol, because before this we just peeked at it.
    (void)getNoEOF(stream);

    return r;
}

template<typename T, typename std::enable_if<std::is_same<T, std::map<std::string, boost::any>>::value, int>::type = 0>
static inline T decode(std::istream &stream)
{
    auto c = getNoEOF(stream);
    if (c != DICTIONARY_CODE) {
        BOOST_THROW_EXCEPTION(decode_code_error() << code_info(c));
    }

    // Read all the keys.
    auto keys = std::vector<std::string>();
    while (peekNoEOF(stream) != MARK_CODE) {
        keys.push_back(decode<std::string>(stream));
    }

    // Read the actual mark symbol, because before this we just peeked at it.
    (void)getNoEOF(stream);

    // Now get all the values in the same order.
    auto r = T();
    for (auto key : keys) {
        r[key] = decode(stream);
    }

    return r;
}

static inline boost::any decode(std::istream &stream)
{
    switch (auto t = peekType(stream)) {
    case Type::None: return decode<boost::none_t>(stream);
    case Type::Boolean: return decode<bool>(stream);
    case Type::Integer: return decode<int64_t>(stream);
    case Type::BinaryFloat: return decode<double>(stream);
    case Type::String: return decode<std::string>(stream);
    case Type::List: return decode<std::vector<boost::any>>(stream);
    case Type::Dictionary: return decode<std::map<std::string, boost::any>>(stream);
    default:
        BOOST_THROW_EXCEPTION(decode_type_error() << type_info(t));
    }
}

template <typename T>
static inline T decode(const std::string &str)
{
    auto str_stream = std::stringstream(str);

    return decode<T>(str_stream);
}

static inline boost::any decode(const std::string &str)
{
    auto str_stream = std::stringstream(str);

    return decode(str_stream);
}


};};};
