

namespace Orion {
namespace Rigel {
namespace CBSON {

static inline size_t encodeLength(int64_t value)
{
    return (sizeof (value) * 8) / 7 + 1;
}

static inline size_t encodeLength(const string &value)
{
    return value.length() + 2;
}

template<class T>
static inline size_t encodeLength(const T &name, const u8string &value)
{
    size_t nr_chunks = (value.length() + 254) / 255;
    return 1 + encodeLength(name) + nr_chunks * 256;
}

static inline size_t encodeLength(const u8string &value)
{
    return encodeLength(none, value);
}

static inline size_t encodeLength(bool value)
{
    return 1;
}

static inline size_t encodeLength(void, none_t value)
{
    return 1;
}

template<class T>
static inline size_t encodeLength(const vector<T> &container)
{
    size_t length = 2;

    for (auto const &item: container) {
        length+= encodeLength(item);
    }

    return length;
}

template<class T, U, V>
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

<class U, V>
static inline size_t encodeLength(const map<U, V> &container)
{
    return encodeLength(none, container);
}

};};};
