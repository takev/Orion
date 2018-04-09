
#include <boost/none.hpp>
#include <cstdint>
#include <vector>
#include <map>

namespace Orion {
namespace Rigel {
namespace CBSON {

using namespace std;
using namespace boost;

static inline size_t encodeLength(int64_t value)
{
    return (sizeof (value) * 8) / 7 + 1;
}

static inline size_t encodeLength(const string &value)
{
    return value.length() + 2;
}

template<typename T>
static inline size_t encodeLength(const T &name, const string &value)
{
    size_t nr_chunks = (value.length() + 254) / 255;
    return 1 + encodeLength(name) + nr_chunks * 256;
}

static inline size_t encodeLength(bool value)
{
    return 1;
}

static inline size_t encodeLength(none_t value)
{
    return 1;
}

template<typename T>
static inline size_t encodeLength(const vector<T> &container)
{
    size_t length = 2;

    for (auto const &item: container) {
        length+= encodeLength(item);
    }

    return length;
}

template<typename T, typename U, typename V>
static inline size_t encodeLength(const T &name, const map<U, V> &container)
{
    size_t length = 3;

    length += encodeLength(name);
    for (auto const &item: container) {
        length+= encodeLength(item.first);
        length+= encodeLength(item.second);
    }

    return length;
}

template<typename U, typename V>
static inline size_t encodeLength(const map<U, V> &container)
{
    return encodeLength(none, container);
}

};};};
