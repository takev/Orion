#pragma once

#include <cstdint>
#include <algorithm>

#include <smmintrin.h>
#include <wmmintrin.h>
#include <nmmintrin.h>
#include <emmintrin.h>


namespace Orion {
namespace Rigel {

static inline __m128i AES128KeyGenPart2(__m128i tmp1, __m128i tmp2)
{
    //pshufd tmp2, tmp2, 0xff
    tmp2 = _mm_shuffle_epi32(tmp2, 0xff);

    //vpslldq tmp3, tmp1, 0x4
    auto tmp3 = _mm_slli_si128(tmp1, 0x4);

    //pxor tmp1, tmp3
    tmp1 = _mm_xor_si128(tmp1, tmp3);

    //vpslldq tmp3, tmp1, 0x4
    tmp3 = _mm_slli_si128(tmp1, 0x4);

    //pxor tmp1, tmp3
    tmp1 = _mm_xor_si128(tmp1, tmp3);

    //vpslldq tmp3, tmp1, 0x4
    tmp3 = _mm_slli_si128(tmp1, 0x4);

    //pxor tmp1, tmp3
    tmp1 = _mm_xor_si128(tmp1, tmp3);

    //pxor tmp1, tmp2
    tmp1 = _mm_xor_si128(tmp1, tmp2);

    return tmp1;
}

template<bool ENCRYPT>
static inline uint32_t AES128XorPartialBlock(uint32_t CRC, size_t nrBytes, __uint128_t &dst, const __uint128_t &src, __m128i cypher)
{
    __uint128_t cypher128;
    auto cypher8 = reinterpret_cast<uint8_t *>(&cypher128);
    auto src8 = reinterpret_cast<const uint8_t *>(&src);
    auto dst8 = reinterpret_cast<uint8_t *>(&dst);

    _mm_store_si128(reinterpret_cast<__m128i *>(&cypher128), cypher);

    for (size_t i = 0; i < nrBytes; i++) {
        auto byte = src8[i];
        if (ENCRYPT) {
            CRC = _mm_crc32_u8(CRC, byte);
            byte ^= cypher8[i];
        } else {
            byte ^= cypher8[i];
            CRC = _mm_crc32_u8(CRC, byte);
        }
        dst8[i] = byte;
    }

    return CRC;
}

template<bool ENCRYPT, bool SCRUB_CRC>
static inline uint32_t AES128XorFullBlock(uint32_t CRC, __uint128_t &dst, const __uint128_t &src, __m128i cypher)
{
    auto block = _mm_load_si128(reinterpret_cast<const __m128i *>(&src));

    uint64_t blockLo;
    uint64_t blockHi;
    if (ENCRYPT) {
        blockLo = _mm_cvtsi128_si64(block);
        blockHi = _mm_extract_epi64(block, 1);
        CRC = _mm_crc32_u64(CRC, blockLo);
        CRC = _mm_crc32_u64(CRC, blockHi);

        block = _mm_xor_si128(block, cypher);
    } else {
        block = _mm_xor_si128(block, cypher);

        blockLo = _mm_cvtsi128_si64(block);
        blockHi = SCRUB_CRC ? _mm_extract_epi32(block, 2) : _mm_extract_epi64(block, 1);

        CRC = _mm_crc32_u64(CRC, blockLo);
        CRC = _mm_crc32_u64(CRC, blockHi);
    }

    _mm_store_si128(reinterpret_cast<__m128i *>(&dst), block);

    return CRC;
}

template<bool ENCRYPT, bool SCRUB_CRC, int X>
static inline uint32_t AES128XorBlock(uint32_t CRC, size_t nrBlocks, size_t nrBytes, __uint128_t &dst, const __uint128_t &src, __m128i cypher)
{
    if (nrBlocks >= X && nrBytes < 16) {
        return AES128XorPartialBlock<ENCRYPT>(CRC, nrBytes, dst, src, cypher);
    } else if (nrBlocks >= X) {
        return AES128XorFullBlock<ENCRYPT, SCRUB_CRC && X == 0>(CRC, dst, src, cypher);
    } else {
        return CRC;
    }
}

class AES128 {
    __uint128_t key;
    __uint128_t keyRounds[11];    

public:
    inline AES128(__uint128_t key) :
        key(key)
    {
        __m128i tmp1 = _mm_load_si128(reinterpret_cast<__m128i *>(&key));
        __m128i tmp2;

#define AES128_KEYGEN_ROUND(x, rcon)\
        tmp2 = _mm_aeskeygenassist_si128(tmp1, rcon);\
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);\
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[x]), tmp1);

        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[0]), tmp1);
        AES128_KEYGEN_ROUND(1, 0x01)
        AES128_KEYGEN_ROUND(2, 0x02)
        AES128_KEYGEN_ROUND(3, 0x04)
        AES128_KEYGEN_ROUND(4, 0x08)
        AES128_KEYGEN_ROUND(5, 0x10)
        AES128_KEYGEN_ROUND(6, 0x20)
        AES128_KEYGEN_ROUND(7, 0x40)
        AES128_KEYGEN_ROUND(8, 0x80)
        AES128_KEYGEN_ROUND(9, 0x1b)
        AES128_KEYGEN_ROUND(10, 0x36)
#undef AES128_KEYGEN_ROUND
    }

    /** Process a buffer of up to 8 128-bit blocks in parralel.
     *
     * @param ENCRYPT true if encrypting, false if decrypting.
     * @param SCRUB_CRC Don't take the CRC in the encrypted message into account
     *        when calculating the CRC. The CRC in the encrypted message is located
     *        in the last 32-bit word of the first 128-bit block.
     * @param CRC The CRC calculated from a previous call.
     * @param counter MMX Register containing a 64-bit counter (lsb), and 64 bit nonce (msb). Counter is incremented
     *        by 64 (4 blocks of 16 bytes) in-place.
     * @param dst Destination buffer. dst and src may alias.
     * @param src Source buffer. dst and src may alias.
     * @param nrBlock Number of 128-bit blocks to process minus 1. nrBlocks must be between 0 and 7 (inclusive).
     * @return CRC-32C Result of the plain-text data.
     */
    template<bool ENCRYPT, bool SCRUB_CRC>
    inline uint32_t AES128CTRProcess8Blocks(uint32_t CRC, __m128i &counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        size_t nrBlocks = (size / sizeof (__uint128_t));
        size_t nrBytes = size % sizeof (__uint128_t);

        __m128i cypher0;
        __m128i cypher1;
        __m128i cypher2;
        __m128i cypher3;
        __m128i cypher4;
        __m128i cypher5;
        __m128i cypher6;
        __m128i cypher7;

        {
            auto key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[0]));
            auto blockSize = _mm_set_epi64x(0, sizeof(__uint128_t));

            cypher0 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
            cypher1 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
            cypher2 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
            cypher3 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
            cypher4 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
            cypher5 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
            cypher6 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
            cypher7 = _mm_xor_si128(counter, key);
            counter = _mm_add_epi64(counter, blockSize);
        }

#define AES128_ROUND(instruction, round)\
        {\
            auto key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[round]));\
            switch (nrBlocks) {\
            case 8:\
            case 7: cypher7 = instruction(cypher7, key);\
            case 6: cypher6 = instruction(cypher6, key);\
            case 5: cypher5 = instruction(cypher5, key);\
            case 4: cypher4 = instruction(cypher4, key);\
            case 3: cypher3 = instruction(cypher3, key);\
            case 2: cypher2 = instruction(cypher2, key);\
            case 1: cypher1 = instruction(cypher1, key);\
            case 0: cypher0 = instruction(cypher0, key);\
            }\
        }

        AES128_ROUND(_mm_aesenc_si128, 1);
        AES128_ROUND(_mm_aesenc_si128, 2);
        AES128_ROUND(_mm_aesenc_si128, 3);
        AES128_ROUND(_mm_aesenc_si128, 4);
        AES128_ROUND(_mm_aesenc_si128, 5);
        AES128_ROUND(_mm_aesenc_si128, 6);
        AES128_ROUND(_mm_aesenc_si128, 7);
        AES128_ROUND(_mm_aesenc_si128, 8);
        AES128_ROUND(_mm_aesenc_si128, 9);
        AES128_ROUND(_mm_aesenclast_si128, 10);
#undef AES128_ROUND

#define AES128_XOR_BLOCK(x)\
        CRC = AES128XorBlock<ENCRYPT, SCRUB_CRC, x>(CRC, nrBlocks, nrBytes, dst[x], src[x], cypher ## x)

        AES128_XOR_BLOCK(0);
        AES128_XOR_BLOCK(1);
        AES128_XOR_BLOCK(2);
        AES128_XOR_BLOCK(3);
        AES128_XOR_BLOCK(4);
        AES128_XOR_BLOCK(5);
        AES128_XOR_BLOCK(6);
        AES128_XOR_BLOCK(7);
#undef AES128_XOR_BLOCK

        return CRC;
    }

    /** Encrypt buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the src buffer.
     */
    inline uint32_t CTREncrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));
        size_t todo = size;
        size_t blockNr = 0;
        uint32_t CRC = 0xffffffff;

        while (todo > 0) {
			auto iterationNrBytes = std::min(todo, 8 * sizeof(__uint128_t));

            CRC = AES128CTRProcess8Blocks<true, false>(CRC, counter, &dst[blockNr], &src[blockNr], iterationNrBytes);

            todo-= iterationNrBytes;
            blockNr+= iterationNrBytes / sizeof(__uint128_t);
        }

        return CRC ^ 0xfffffff;
    }

    /** Decrypt buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the dst buffer.
     */
    template<bool SCRUB_CRC>
    uint32_t CTRDecrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));
        size_t todo = size;
        size_t blockNr = 0;
        uint32_t CRC = 0xffffffff;

		if (SCRUB_CRC) {
			auto iterationNrBytes = std::min(todo, 8 * sizeof(__uint128_t));

			CRC = AES128CTRProcess8Blocks<false, true>(CRC, counter, &dst[blockNr], &src[blockNr], iterationNrBytes);

            todo-= iterationNrBytes;
            blockNr+= iterationNrBytes / sizeof(__uint128_t);
		}

        if (todo > 0) {
			auto iterationNrBytes = std::min(todo, 8 * sizeof(__uint128_t));

            CRC = AES128CTRProcess8Blocks<false, false>(CRC, counter, &dst[blockNr], &src[blockNr], iterationNrBytes);

            todo-= iterationNrBytes;
            blockNr+= iterationNrBytes / sizeof(__uint128_t);
        }

        return CRC ^ 0xffffffff;
    }
};

uint32_t AES128CompilerTest(__uint128_t key, __uint128_t counter, __uint128_t *buffer, size_t buffer_size)
{
    auto C = AES128(key);

    auto CRC = C.CTRDecrypt<true>(counter, buffer, buffer, buffer_size);

    return CRC;
}



};};
