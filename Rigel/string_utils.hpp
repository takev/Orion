
#include <string>
#include <string.h>

namespace Orion {
namespace Rigel {

inline bool startswith(const std::string &haystack, const std::string &needle)
{
    if (haystack.size() < needle.size()) {
        return false;
    } else {
        return memcmp(haystack.data(), needle.data(), needle.size()) == 0;
    }
}


};};
