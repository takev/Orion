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
     * @param nrBlock Number of 128-bit blocks to process. nrBlocks must be between 1 and 8 (inclusive).
     * @return CRC-32C Result of the plain-text data.
     */
    template<bool ENCRYPT, bool SCRUB_CRC>
    inline uint32_t AES128CTRProcess8Blocks(uint32_t CRC, __m128i &counter, __uint128_t *dst, const __uint128_t *src, int nrBlocks)
    {
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

#define AES128_ROUND0(x)\
        {\
            auto offset = _mm_set_epi64x(0, x * sizeof(__uint128_t));\
            cypher ## x = _mm_add_epi64(counter, offset);\
            cypher ## x = _mm_xor_si128(cypher ## x, key);\
        }

            switch (nrBlocks) {
            case 8: AES128_ROUND0(7)
            case 7: AES128_ROUND0(6)
            case 6: AES128_ROUND0(5)
            case 5: AES128_ROUND0(4)
            case 4: AES128_ROUND0(3)
            case 3: AES128_ROUND0(2)
            case 2: AES128_ROUND0(1)
            case 1: AES128_ROUND0(0)
            default: break;
            }
#undef AES128_ROUND0
        }

#define AES128_ROUND(instruction, x)\
        {\
            auto key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[x]));\
            switch (nrBlocks) {\
            case 8: cypher7 = instruction(cypher7, key);\
            case 7: cypher6 = instruction(cypher6, key);\
            case 6: cypher5 = instruction(cypher5, key);\
            case 5: cypher4 = instruction(cypher4, key);\
            case 4: cypher3 = instruction(cypher3, key);\
            case 3: cypher2 = instruction(cypher2, key);\
            case 2: cypher1 = instruction(cypher1, key);\
            case 1: cypher0 = instruction(cypher0, key);\
            default: break;\
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
        {\
            auto block = _mm_load_si128(reinterpret_cast<const __m128i *>(&src[x]));\
            if (ENCRYPT) {\
                auto blockLo = _mm_cvtsi128_si64(block);\
                auto blockHi = _mm_extract_epi64(block, 1);\
                CRC = _mm_crc32_u64(CRC, blockLo);\
                CRC = _mm_crc32_u64(CRC, blockHi);\
                block = _mm_xor_si128(block, cypher ## x);\
            } else {\
                block = _mm_xor_si128(block, cypher ## x);\
                auto blockLo = _mm_cvtsi128_si64(block);\
                auto blockHi = _mm_extract_epi64(block, 1);\
                if (SCRUB_CRC && x == 0) { blockHi &= 0x00000000ffffffff; }\
                CRC = _mm_crc32_u64(CRC, blockLo);\
                CRC = _mm_crc32_u64(CRC, blockHi);\
            }\
            _mm_store_si128(reinterpret_cast<__m128i *>(&dst[x]), block);\
        }

        if (nrBlocks >= 1) AES128_XOR_BLOCK(0)
        if (nrBlocks >= 2) AES128_XOR_BLOCK(1)
        if (nrBlocks >= 3) AES128_XOR_BLOCK(2)
        if (nrBlocks >= 4) AES128_XOR_BLOCK(3)
        if (nrBlocks >= 5) AES128_XOR_BLOCK(4)
        if (nrBlocks >= 6) AES128_XOR_BLOCK(5)
        if (nrBlocks >= 7) AES128_XOR_BLOCK(6)
        if (nrBlocks == 8) AES128_XOR_BLOCK(7)
#undef AES128_XOR_BLOCK

        auto offset = _mm_set_epi64x(0, nrBlocks * sizeof(__uint128_t));\
        counter = _mm_add_epi64(counter, offset);

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
    inline uint32_t CTREncrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t nrBlocks)
    {
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));
        size_t todo = nrBlocks;
        size_t done = 0;
        uint32_t CRC = 0xffffffff;

        while (todo >= 8) {
            CRC = AES128CTRProcess8Blocks<true, false>(CRC, counter, &dst[done], &src[done], 8);
            todo-= 8;
            done+= 8;
        }

        CRC = AES128CTRProcess8Blocks<true, false>(CRC, counter, &dst[done], &src[done], todo);

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
    uint32_t CTRDecrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t nrBlocks)
    {
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));
        size_t todo = nrBlocks;
        size_t done = 0;
        uint32_t CRC = 0xffffffff;

        auto firstNrBlocks = std::min(todo, static_cast<size_t>(8));
        CRC = AES128CTRProcess8Blocks<false, SCRUB_CRC>(CRC, counter, &dst[done], &src[done], firstNrBlocks);
        done += firstNrBlocks;
        todo -= firstNrBlocks;

        if (todo >= 8) {
            CRC = AES128CTRProcess8Blocks<false, false>(CRC, counter, &dst[done], &src[done], 8);
            todo-= 8;
            done+= 8;
        }

        CRC = AES128CTRProcess8Blocks<false, false>(CRC, counter, &dst[done], &src[done], todo);

        return CRC ^ 0xffffffff;
    }
};

uint32_t AES128CompilerTest(__uint128_t key, __uint128_t counter, __uint128_t *buffer, size_t buffer_size)
{
    auto C = AES128(key);

    auto CRC = C.CTREncrypt(counter, buffer, buffer, buffer_size);

    return CRC;
}



};};
