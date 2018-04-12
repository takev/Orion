

namespace Orion {
namespace Alnitak {

enum class PropertyType : uint32_t {
};

enum class PropertyRule : uint8_t {
};

typedef union {
    uint32_t    u32;
    float       f32;
} conv32_t;

struct Property {
    PropertyType    type;
    PropertyRule    rule;
    float           value;

    inline Property(uint64_t x) {
        conv32_t conv;

        conv.u32 = x >> 32;

        value = conv.f32;
        type = static_cast<PropertyType>(x & 0xffffff);
        rule = static_cast<PropertyRule>((x >> 24) & 0xff);
    }

    inline uint64_t pack(void) {
        conv32_t conv;

        conv.f32 = value;

        return (conv.u32 << 32) | (rule << 24) | (type);
    }
}



};};
