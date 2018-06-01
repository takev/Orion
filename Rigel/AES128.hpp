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

template<int BLOCK_NR>
static inline __m128i AES128Round0(__m128i &counter, __m128i blockSize, __m128i key, size_t nrBlocks)
{
    if (BLOCK_NR < nrBlocks) {
        auto cypher = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        return cypher;
    } else {
        return _mm_undefined_si128();
    }
}

template<bool ENCRYPT, int BLOCK_NR>
static inline uint32_t AES128XorFullBlock(uint32_t CRC, size_t nrBlocks, __uint128_t *dst, const __uint128_t *src, __m128i cypher)
{
    if (BLOCK_NR < nrBlocks) {
        auto block = _mm_load_si128(reinterpret_cast<const __m128i *>(src));

        if (ENCRYPT) {
            auto blockLo = _mm_cvtsi128_si64(block);
            auto blockHi = _mm_extract_epi64(block, 1);
            CRC = _mm_crc32_u64(CRC, blockLo);
            CRC = _mm_crc32_u64(CRC, blockHi);

            block = _mm_xor_si128(block, cypher);
        } else {
            block = _mm_xor_si128(block, cypher);

            auto blockLo = _mm_cvtsi128_si64(block);
            auto blockHi = _mm_extract_epi64(block, 1);

            CRC = _mm_crc32_u64(CRC, blockLo);
            CRC = _mm_crc32_u64(CRC, blockHi);
        }

        _mm_store_si128(reinterpret_cast<__m128i *>(dst), block);
    }
    return CRC;
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

    template<bool ENCRYPT, ssize_t CRC32_LOCATION=-1>
    inline uint32_t AES128CTRProcessPartialBlock(uint32_t CRC, __m128i &counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        __m128i cypher;

        auto key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[0]));
        auto blockSize = _mm_set_epi64x(0, size);

        cypher = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);

        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[1]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[2]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[3]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[4]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[5]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[6]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[7]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[8]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[9]));
        cypher = _mm_aesenc_si128(cypher, key);
        key = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[10]));
        cypher = _mm_aesenclast_si128(cypher, key);

        __uint128_t cypher128;
        auto cypher8 = reinterpret_cast<const uint8_t *>(&cypher128);
        auto src8 = reinterpret_cast<const uint8_t *>(src);
        auto dst8 = reinterpret_cast<uint8_t *>(dst);

        _mm_store_si128(reinterpret_cast<__m128i *>(&cypher128), cypher);
        for (size_t i = 0; i < size; i++) {
            auto byte = src8[i];

            if (ENCRYPT) {
                CRC = _mm_crc32_u8(CRC, byte);
                byte ^= cypher8[i];

            } else {
                byte ^= cypher8[i];
                if (
                    (CRC32_LOCATION >= 0) &&
                    (i >= static_cast<size_t>(CRC32_LOCATION)) &&
                    (i < static_cast<size_t>(CRC32_LOCATION) * sizeof (uint32_t))
                ) {
                    // Ignore the CRC that was encrypted.
                    CRC = _mm_crc32_u8(CRC, 0);
                } else {
                    CRC = _mm_crc32_u8(CRC, byte);
                }
            }

            dst8[i] = byte;
        }

        return CRC;
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
     * @param nrBlocks Number of 128-bit blocks to process. nrBlocks must be between 1 and 8 (inclusive).
     * @return CRC-32C Result of the plain-text data.
     */
    template<bool ENCRYPT>
    inline uint32_t AES128CTRProcess8FullBlocks(uint32_t CRC, __m128i &counter, __uint128_t *dst, const __uint128_t *src, size_t nrBlocks)
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
            auto blockSize = _mm_set_epi64x(0, sizeof(__uint128_t));

            cypher0 = AES128Round0<0>(counter, blockSize, key, nrBlocks);
            cypher1 = AES128Round0<1>(counter, blockSize, key, nrBlocks);
            cypher2 = AES128Round0<2>(counter, blockSize, key, nrBlocks);
            cypher3 = AES128Round0<3>(counter, blockSize, key, nrBlocks);
            cypher4 = AES128Round0<4>(counter, blockSize, key, nrBlocks);
            cypher5 = AES128Round0<5>(counter, blockSize, key, nrBlocks);
            cypher6 = AES128Round0<6>(counter, blockSize, key, nrBlocks);
            cypher7 = AES128Round0<7>(counter, blockSize, key, nrBlocks);
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

        CRC = AES128XorFullBlock<ENCRYPT, 0>(CRC, nrBlocks, &dst[0], &src[0], cypher0);
        CRC = AES128XorFullBlock<ENCRYPT, 1>(CRC, nrBlocks, &dst[1], &src[1], cypher1);
        CRC = AES128XorFullBlock<ENCRYPT, 2>(CRC, nrBlocks, &dst[2], &src[2], cypher2);
        CRC = AES128XorFullBlock<ENCRYPT, 3>(CRC, nrBlocks, &dst[3], &src[3], cypher3);
        CRC = AES128XorFullBlock<ENCRYPT, 4>(CRC, nrBlocks, &dst[4], &src[4], cypher4);
        CRC = AES128XorFullBlock<ENCRYPT, 5>(CRC, nrBlocks, &dst[5], &src[5], cypher5);
        CRC = AES128XorFullBlock<ENCRYPT, 6>(CRC, nrBlocks, &dst[6], &src[6], cypher6);
        CRC = AES128XorFullBlock<ENCRYPT, 7>(CRC, nrBlocks, &dst[7], &src[7], cypher7);
        return CRC;
    }

    /** Process (encrypt/decrypt) buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the src buffer.
     */
    template<bool ENCRYPT, ssize_t CRC32_LOCATION=-1>
    inline uint32_t CTRProcess(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        uint32_t CRC = 0xffffffff;
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));

        size_t todoBytes = size;
        size_t todoBlocks = size / sizeof (__uint128_t);
        size_t doneBlocks = 0;

        if (CRC32_LOCATION >= 0) {
            // XXX assert(size >= CRC32_LOCATION + sizeof (uint32));

            // Process all the full blocks, up to 8, before the block with the CRC inside it.
            const size_t CRC32_BLOCK_LOCATION = CRC32_LOCATION / sizeof (__uint128_t);
            while (doneBlocks < CRC32_BLOCK_LOCATION) {
                auto nrBlocks = std::min(todoBlocks, static_cast<size_t>(8));

                CRC = AES128CTRProcess8FullBlocks<ENCRYPT>(CRC, counter, &dst[doneBlocks], &src[doneBlocks], nrBlocks);

                todoBytes -= nrBlocks * sizeof (__uint128_t);
                todoBlocks -= nrBlocks;
                doneBlocks += nrBlocks;
            }

            // The CRC is located inside this block. Most often this is a full block, but we use this function
            // to mask the CRC during decryption.
            auto nrBytes = std::min(todoBytes, sizeof (__uint128_t));
            CRC = AES128CTRProcessPartialBlock<ENCRYPT,CRC32_LOCATION % sizeof (__uint128_t)>(CRC, counter, &dst[doneBlocks], &src[doneBlocks], nrBytes);

            todoBytes -= nrBytes;
            if (nrBytes == sizeof (__uint128_t)) {
                todoBlocks -= 1;
            }
        }

        // Process all the full blocks, up to 8, until the end of the data.
        while (todoBlocks > 0) {
			auto nrBlocks = std::min(todoBlocks, static_cast<size_t>(8));

            CRC = AES128CTRProcess8FullBlocks<ENCRYPT>(CRC, counter, &dst[doneBlocks], &src[doneBlocks], nrBlocks);

            todoBytes -= nrBlocks * sizeof (__uint128_t);
            todoBlocks -= nrBlocks;
            doneBlocks += nrBlocks;
        }

        // Process the last few bytes in the last block.
        if (todoBytes) {
            CRC = AES128CTRProcessPartialBlock<ENCRYPT>(CRC, counter, &dst[doneBlocks], &src[doneBlocks], todoBytes);
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
    template<bool CRC32_LOCATION=-1>
    uint32_t CTRDecrypt(__uint128_t counter, void *dst, const void *src, size_t size)
    {
        // XXX assert dst and src are aligned to 128 bits.

        auto *dst128 = reinterpret_cast<__uint128_t *>(dst);
        auto *src128 = reinterpret_cast<const __uint128_t *>(src);

        return CTRProcess<false, CRC32_LOCATION>(counter, dst128, src128, size);
    }

    uint32_t CTREncrypt(__uint128_t counter, void *dst, const void *src, size_t size)
    {
        // XXX assert dst and src are aligned to 128 bits.

        auto *dst128 = reinterpret_cast<__uint128_t *>(dst);
        auto *src128 = reinterpret_cast<const __uint128_t *>(src);

        return CTRProcess<true>(counter, dst128, src128, size);
    }
};

uint32_t AES128CompilerTest(__uint128_t key, __uint128_t counter, __uint128_t *buffer, size_t buffer_size)
{
    auto C = AES128(key);

    auto CRC = C.CTRDecrypt(counter, buffer, buffer, buffer_size);

    return CRC;
}



};};
