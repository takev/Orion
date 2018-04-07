

namespace Orion {
namespace Rigel {
namespace CBSON {

const uint8_t MARK_CODE = 0x00;
const uint8_t NONE_CODE = 0x10;
const uint8_t TRUE_CODE = 0x11;
const uint8_t FALSE_CODE = 0x12;
const uint8_t LIST_CODE = 0x13;
const uint8_t OBJECT_CODE = 0x14;
const uint8_t DECIMAL_FLOAT_CODE = 0x15;
const uint8_t BINARY_FLOAT_CODE = 0x16;
const uint8_t BYTE_ARRAY_CODE = 0x17;
const uint8_t UTF8_STRING_CODE = 0x18;
const uint8_t RESERVED1_CODE = 0x19;
const uint8_t RESERVED2_CODE = 0x1a;

static inline void encode(u8string &buffer, int64_t value)
{
    int64_t end_conv = value >= 0 ? 0 : -1;
    uint8_t end_top_bit = value >= 0 ? 0 : 1;
    uint8_t part;
    uint8_t top_bit;
    bool more;
   
    // We need the signed integer for sign extended shift, and the unsigned integer
    // to grab bits from.
    conv64_t conv;
    conv.i64 = value;

    // First part is 6 bits + continue-bit
    part = conv.u64 & 0x3f;
    conv.i64 >> 6;
    top_bit = part >> 5;
    more = conv.i64 != end_conv || top_bit != end_top_bit;

    buffer+= (more ? 0xc0 : 0x80) | part;

    // following parts are 7 bits + continue-bit
    while (more) {
        part = conv.u64 & 0x7f;
        conv.i64 >> 7;
        top_bit = part >> 6;
        more = conv.i64 != end_conv || top_bit != end_top_bit;

        buffer+= (more ? 0x80 : 0x00) | part;
    }
}

static inline void encode(u8string &buffer, const string &value)
{
    bool is_ascii = true;
    size_t string_start = buffer.length();

    // An ASCII string must have at least two characters.
    // Because the first character has the stop-bit unset.
    if (value.length() < 2) {
        is_ascii = false;
        buffer+= UTF8_CODE;
    }

    // Copy characters to the buffer, and insert a start-code
    // when the string contains utf-8 characters.
    for (uint8_t &c : value) {
        buffer+= c;
        if (c == 0x00) {
            throw out_of_range("A UTF-8 encoded string may not contain a NUL character");
        }
        if ((c >= 0x80 || (x >= 0x10 && x <= 0x1a)) && is_ascii) {
            is_ascii = false;
            buffer.insert(string_start, UTF8_CODE);
        }
    }

    if (is_ascii) {
        // ASCII strings must be terminated by a stop-bit on the
        // last character.
        buffer.last() |= 0x80;
    } else {
        // UTF-8 string and empty strings must be NULL terminated.
        buffer+= 0x00;
    }
}

template<class T>
static inline void encode(u8string &buffer, const T &name, const u8string &value)
{
    buffer+= BYTE_ARRAY_CODE;
    encode(buffer, name);

    size_t nr_full_chunks = value.length() / 255;
    size_t last_chunk_size = value.length() % 255;

    // Copy chunks of 255 bytes length.
    for (off_t chunk = 0; chunk < nr_full_chunks; chunk++) {
        buffer+= 0xff;
        buffer.append(value, chunk * 255, 255);
    }

    // Last chunk, with length less than 255, including zero.
    buffer += last_chunk_size;
    buffer.append(value, value.length() - last_chunk_size, last_chunk_size);
}

static inline void encode(u8string &buffer, const u8string &value)
{
    encode(buffer, none, value);
}

static inline void encode(u8string &buffer, bool value)
{
    buffer+= value ? TRUE_CODE : FALSE_CODE;
}

static inline void encode(u8string &buffer, none_t value)
{
    buffer+= NONE_CODE;
}

template<class T>
static inline void encode(u8string &buffer, const vector<T> &container)
{
    buffer+= LIST_CODE;

    for (auto const &item: container) {
        encode(buffer, item);
    }

    buffer+= MARK_CODE;
}

template<class T, U, V>
static inline void encode(u8string &buffer, const T &name, const map<U, V> &container)
{
    buffer+= LIST_CODE;
    encode(buffer, name);

    // Maps are already sorted by key.
    for (auto &item: container) {
        encode(buffer, item.first);
    }

    buffer+= MARK_CODE;

    for (auto &item: container) {
        encode(buffer, item.second);
    }

    buffer+= MARK_CODE;
}

template<class U, V>
static inline void encode(u8string &buffer, const map<U, V> &container)
{
    encode(buffer, none, container);
}

};};};
