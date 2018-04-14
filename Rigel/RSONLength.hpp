
#include <boost/none.hpp>
#include <cstdint>
#include <vector>
#include <map>

namespace Orion {
namespace Rigel {
namespace RSON {

template<typename T>
static inline size_t lengthInteger(T value)
{
    return (sizeof (value) * 8) / 7 + 1;
}

static inline size_t length(__int128_t v) { return lengthInteger(v); }
static inline size_t length(int64_t v) { return lengthInteger(v); }
static inline size_t length(int32_t v) { return lengthInteger(v); }
static inline size_t length(int16_t v) { return lengthInteger(v); }
static inline size_t length(int8_t v) { return lengthInteger(v); }

static inline size_t length(__uint128_t v) { return lengthInteger(v); }
static inline size_t length(uint64_t v) { return lengthInteger(v); }
static inline size_t length(uint32_t v) { return lengthInteger(v); }
static inline size_t length(uint16_t v) { return lengthInteger(v); }
static inline size_t length(uint8_t v) { return lengthInteger(v); }

template<typename T>
static inline size_t lengthFloat(T value)
{
    return 1 + length(static_cast<int64_t>(0)) + length(static_cast<int32_t>(0));
}

static inline size_t length(float v) { return lengthFloat(v); }
static inline size_t length(double v) { return lengthFloat(v); }

static inline size_t length(const std::string &value)
{
    return value.length() + 2;
}

static inline size_t length(const std::basic_string<uint8_t> &value)
{
    size_t nr_chunks = (value.length() + 254) / 255;
    return 1 + nr_chunks * 256;
}

static inline size_t length(bool value)
{
    return 1;
}

static inline size_t length(boost::none_t value)
{
    return 1;
}

template<typename U, typename V>
static inline size_t length(const std::map<U, V> &container);

template<typename T>
static inline size_t length(const std::vector<T> &container)
{
    size_t r = 2;

    for (auto const &item: container) {
        r+= length(item);
    }

    return r;
}

template<typename U, typename V>
static inline size_t length(const std::map<U, V> &container)
{
    size_t r = 3;

    for (auto const &item: container) {
        r+= length(item.first);
        r+= length(item.second);
    }

    return r;
}

};};};
