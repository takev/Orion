/* Copyright 2018 Tjienta Vara
 * This file is part of Orion.
 *
 * Orion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Orion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Orion.  If not, see <http://www.gnu.org/licenses/>.
 */
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
