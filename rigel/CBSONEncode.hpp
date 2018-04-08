
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

static inline void encode(ostream &stream, int64_t value)
{
    int64_t end_value = value >= 0 ? 0 : -1;
    uint8_t end_top_bit = value >= 0 ? 0 : 1;
    uint8_t part;
    uint8_t top_bit;
    bool more;
   
    // First part is 6 bits + continue-bit
    part = value & 0x3f;
    top_bit = part >> 5;
    value >> 6;
    more = value != end_value || top_bit != end_top_bit;

    stream.put((more ? 0xc0 : 0x80) | part);

    // following parts are 7 bits + continue-bit
    while (more) {
        part = value & 0x7f;
        top_bit = part >> 6;
        value >> 7;
        more = value != end_value || top_bit != end_top_bit;

        stream.put((more ? 0x80 : 0x00) | part);
    }
}

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
    stream.put(value[value.length() - 1] & 0x80);
    return;

utf8_string_encoding:
    stream.put(UTF8_STRING_CODE);
    stream.write(value.data(), value.length());
    stream.put(MARK_CODE);
}

template<typename T>
static inline void encode(ostream &stream, const T &name, const string &value)
{
    stream.put(BYTE_ARRAY_CODE);
    encode(stream, name);

    size_t nr_full_chunks = value.length() / 255;
    size_t last_chunk_size = value.length() % 255;

    // Copy chunks of 255 bytes length.
    for (off_t chunk = 0; chunk < nr_full_chunks; chunk++) {
        stream.put(0xff);
        stream.write(&value.data()[chunk * 255], 255);
    }

    // Last chunk, with length less than 255, including zero.
    stream.put(last_chunk_size);
    stream.write(&value.data()[value.length() - last_chunk_size], last_chunk_size);
}

static inline void encode(ostream &stream, bool value)
{
    stream.put(value ? TRUE_CODE : FALSE_CODE);
}

static inline void encode(ostream &stream, none_t value)
{
    stream.put(NONE_CODE);
}

template<typename T>
static inline void encode(ostream &stream, const vector<T> &container)
{
    stream.put(LIST_CODE);

    for (auto const &item: container) {
        encode(stream, item);
    }

    stream.put(MARK_CODE);
}

template<typename T, typename U, typename V>
static inline void encode(ostream &stream, const T &name, const map<U, V> &container)
{
    stream.put(LIST_CODE);
    encode(stream, name);

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

template<typename U, typename V>
static inline void encode(ostream &stream, const map<U, V> &container)
{
    encode(stream, none, container);
}

};};};
