#pragma once
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
     */
    SHA512(void);

    /** Constructor
     * This version allows data to be added directly.
     *
     * @param buffer The bytes to hash.
     * @param bufferSize The amount of bytes to hash.
     */
    SHA512(const char *buffer, size_t bufferSize);

    /** Constructor
     * This version allows data to be added directly.
     *
     * @param buffer The bytes to hash.
     * @param bufferSize The amount of bytes to hash.
     */
    SHA512(const std::string &data);

    /** Add some data.
     *
     * @param buffer The bytes to hash.
     * @param bufferSize The amount of bytes to hash.
     */
    void add(const char *buffer, size_t bufferSize);

    /** Add some data.
     *
     * @param buffer The bytes to hash.
     * @param bufferSize The amount of bytes to hash.
     */
    void add(const std::string &data);

    /** Finish.
     * Hash the last bit of data and add padding.
     *
     * @return The 512 bits hash value.
     */
    BigInt<512> finish(void);
};

};};
