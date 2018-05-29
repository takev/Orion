
#include <wmmintrin.h>

#include "BigInt.hpp"
#include "int_utils.hpp"

namespace Orion {
namespace Rigel {

/** SHA512
 */
class SHA512 {
public:
    BigInt<512> state;
    __uint128_t dataSize;
    size_t overflowBufferSize;
    char overflowBuffer[127];

    /** Constructor
     *
     * @param salt The salt to be used with the data.
     */
    SHA512(void);

    SHA512(const char *buffer, size_t bufferSize);

    SHA512(const std::string &data);

    /** Add some data.
     *
     * @param buffer The bytes to hash.
     * @param bufferSize The amount of bytes to hash.
     */
    void add(const char *buffer, size_t bufferSize);

    void add(const std::string &data);

    /** Finish.
     * Hash the last bit of data and add padding.
     *
     * @return The 256 bits hash value.
     */
    BigInt<512> finish(void);
};

};};
