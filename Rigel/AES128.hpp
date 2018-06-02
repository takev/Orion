#pragma once

#include <cstdint>
#include <algorithm>
#include <string>

#include <smmintrin.h>
#include <wmmintrin.h>
#include <nmmintrin.h>
#include <emmintrin.h>

#include "int_utils.hpp"

namespace Orion {
namespace Rigel {

class AES128 {
    __uint128_t key;

    template<int BLOCK_NR>
    static inline __m128i Round0(__m128i &counter, __m128i key, size_t nrBlocks)
    {
        __m128i cypher;

        if (BLOCK_NR < nrBlocks) {
            cypher = _mm_xor_si128(counter, key);

            auto byte_swap_mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
            counter = _mm_shuffle_epi8(counter, byte_swap_mask);

            // Full 128-bit increment-by-one.
            // compare =   hi, lo     hi, ff
            // check   =   hi, ff     hi, ff
            // cmpeq   =   ff, 00     ff, ff
            // one     =   00, ff     ff, ff
            auto check = _mm_insert_epi64(counter, 0xffffffffffffffff, 0);
            auto reverse_one = _mm_cmpeq_epi64(counter, check);
            auto one = _mm_shuffle_epi32(reverse_one, _MM_SHUFFLE(1, 0, 3, 2));
            counter = _mm_sub_epi64(counter, one);

            counter = _mm_shuffle_epi8(counter, byte_swap_mask);
        }
        return cypher;
    }

    template<bool ENCRYPT, int BLOCK_NR>
    static inline uint32_t XorFullBlock(uint32_t CRC, size_t nrBlocks, __uint128_t *dst, const __uint128_t *src, __m128i cypher)
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

    template<int RCON>
    static inline __m128i KeyExpansion(__m128i prev_key)
    {
        // aeskeyassist xmm2 = xmm1, RCON
        auto tmp1 = _mm_aeskeygenassist_si128(prev_key, RCON);

        // pshufd xmm2 = xmm2, 0xff
        auto tmp2 = _mm_shuffle_epi32(tmp1, 0xff);

        // vpslldq xmm3 = xmm1, 0x4
        auto tmp3 = _mm_slli_si128(prev_key, 4);

        // pxor xmm1 = xmm1, xmm3
        auto tmp4 = _mm_xor_si128(prev_key, tmp3);

        // vpslldq xmm3 = xmm1, 0x4
        auto tmp5 = _mm_slli_si128(tmp4, 4);

        // pxor xmm1 = xmm1, xmm3
        auto tmp6 = _mm_xor_si128(tmp4, tmp5);

        // vpslldq xmm3 = xmm1, 0x4
        auto tmp7 = _mm_slli_si128(tmp6, 4);

        // pxor xmm1 = xmm1, xmm3
        auto tmp8 = _mm_xor_si128(tmp6, tmp7);

        // pxor xmm1 = xmm1, xmm2 
        auto key = _mm_xor_si128(tmp8, tmp2);
        return key;
    }

    template<bool ENCRYPT, ssize_t CRC32_LOCATION=-1>
    static inline uint32_t CTRProcessPartialBlock(uint32_t CRC, __m128i key, __m128i &counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        auto cypher = AES128::Round0<0>(counter, key, 1);
        key = AES128::KeyExpansion<0x01>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x02>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x04>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x08>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x10>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x20>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x40>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x80>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x1b>(key);
        cypher = _mm_aesenc_si128(cypher, key);
        key = AES128::KeyExpansion<0x36>(key);
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
    static inline uint32_t CTRProcess8FullBlocks(uint32_t CRC, __m128i key, __m128i &counter, __uint128_t *dst, const __uint128_t *src, size_t nrBlocks)
    {
        auto cypher0 = AES128::Round0<0>(counter, key, nrBlocks);
        auto cypher1 = AES128::Round0<1>(counter, key, nrBlocks);
        auto cypher2 = AES128::Round0<2>(counter, key, nrBlocks);
        auto cypher3 = AES128::Round0<3>(counter, key, nrBlocks);
        auto cypher4 = AES128::Round0<4>(counter, key, nrBlocks);
        auto cypher5 = AES128::Round0<5>(counter, key, nrBlocks);
        auto cypher6 = AES128::Round0<6>(counter, key, nrBlocks);
        auto cypher7 = AES128::Round0<7>(counter, key, nrBlocks);

#define AES128_ROUND(instruction, RCON)\
        {\
            key = AES128::KeyExpansion<RCON>(key);\
            switch (nrBlocks) {\
            case 8: cypher7 = instruction(cypher7, key);\
            case 7: cypher6 = instruction(cypher6, key);\
            case 6: cypher5 = instruction(cypher5, key);\
            case 5: cypher4 = instruction(cypher4, key);\
            case 4: cypher3 = instruction(cypher3, key);\
            case 3: cypher2 = instruction(cypher2, key);\
            case 2: cypher1 = instruction(cypher1, key);\
            case 1: cypher0 = instruction(cypher0, key);\
            }\
        }

        AES128_ROUND(_mm_aesenc_si128,     0x01);
        AES128_ROUND(_mm_aesenc_si128,     0x02);
        AES128_ROUND(_mm_aesenc_si128,     0x04);
        AES128_ROUND(_mm_aesenc_si128,     0x08);
        AES128_ROUND(_mm_aesenc_si128,     0x10);
        AES128_ROUND(_mm_aesenc_si128,     0x20);
        AES128_ROUND(_mm_aesenc_si128,     0x40);
        AES128_ROUND(_mm_aesenc_si128,     0x80);
        AES128_ROUND(_mm_aesenc_si128,     0x1b);
        AES128_ROUND(_mm_aesenclast_si128, 0x36);
#undef AES128_ROUND

        CRC = AES128::XorFullBlock<ENCRYPT, 0>(CRC, nrBlocks, &dst[0], &src[0], cypher0);
        CRC = AES128::XorFullBlock<ENCRYPT, 1>(CRC, nrBlocks, &dst[1], &src[1], cypher1);
        CRC = AES128::XorFullBlock<ENCRYPT, 2>(CRC, nrBlocks, &dst[2], &src[2], cypher2);
        CRC = AES128::XorFullBlock<ENCRYPT, 3>(CRC, nrBlocks, &dst[3], &src[3], cypher3);
        CRC = AES128::XorFullBlock<ENCRYPT, 4>(CRC, nrBlocks, &dst[4], &src[4], cypher4);
        CRC = AES128::XorFullBlock<ENCRYPT, 5>(CRC, nrBlocks, &dst[5], &src[5], cypher5);
        CRC = AES128::XorFullBlock<ENCRYPT, 6>(CRC, nrBlocks, &dst[6], &src[6], cypher6);
        CRC = AES128::XorFullBlock<ENCRYPT, 7>(CRC, nrBlocks, &dst[7], &src[7], cypher7);
        return CRC;
    }

public:
    inline AES128(__uint128_t key) : key(key) {}

    /** Process (encrypt/decrypt) buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the src buffer.
     */
    template<bool ENCRYPT, ssize_t CRC32_LOCATION=-1>
    uint32_t CTRProcess(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        uint32_t CRC = 0xffffffff;
        auto key = _mm_load_si128(reinterpret_cast<__m128i *>(&this->key));
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

                CRC = AES128::CTRProcess8FullBlocks<ENCRYPT>(CRC, key, counter, &dst[doneBlocks], &src[doneBlocks], nrBlocks);

                todoBytes -= nrBlocks * sizeof (__uint128_t);
                todoBlocks -= nrBlocks;
                doneBlocks += nrBlocks;
            }

            // The CRC is located inside this block. Most often this is a full block, but we use this function
            // to mask the CRC during decryption.
            auto nrBytes = std::min(todoBytes, sizeof (__uint128_t));
            CRC = AES128::CTRProcessPartialBlock<ENCRYPT,CRC32_LOCATION % sizeof (__uint128_t)>(CRC, key, counter, &dst[doneBlocks], &src[doneBlocks], nrBytes);

            todoBytes -= nrBytes;
            if (nrBytes == sizeof (__uint128_t)) {
                todoBlocks -= 1;
            }
        }

        // Process all the full blocks, up to 8, until the end of the data.
        while (todoBlocks > 0) {
			auto nrBlocks = std::min(todoBlocks, static_cast<size_t>(8));

            CRC = AES128::CTRProcess8FullBlocks<ENCRYPT>(CRC, key, counter, &dst[doneBlocks], &src[doneBlocks], nrBlocks);

            todoBytes -= nrBlocks * sizeof (__uint128_t);
            todoBlocks -= nrBlocks;
            doneBlocks += nrBlocks;
        }

        // Process the last few bytes in the last block.
        if (todoBytes) {
            CRC = AES128::CTRProcessPartialBlock<ENCRYPT>(CRC, key, counter, &dst[doneBlocks], &src[doneBlocks], todoBytes);
        }

        return CRC ^ 0xffffffff;
    }

    /** Decrypt buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the dst buffer.
     */
    template<bool ENCRYPT, ssize_t CRC32_LOCATION=-1>
    uint32_t CTRProcess(__uint128_t counter, void *dst, const void *src, size_t size)
    {
        const __uint128_t *src128;
        __uint128_t *dst128;

        if (isAligned<__uint128_t>(src)) {
            src128 = reinterpret_cast<const __uint128_t *>(src);
        } else {
            auto tmp = new __uint128_t[nrItems<__uint128_t>(size)];
            memcpy(tmp, src, size);
            src128 = reinterpret_cast<const __uint128_t *>(tmp);
        }

        if (isAligned<__uint128_t>(dst)) {
            dst128 = reinterpret_cast<__uint128_t *>(dst);
        } else {
            dst128 = new __uint128_t[nrItems<__uint128_t>(size)];
        }

        auto CRC = CTRProcess<ENCRYPT, CRC32_LOCATION>(counter, dst128, src128, size);

        if (!isAligned<__uint128_t>(src)) {
            delete[] src128;
        }

        if (!isAligned<__uint128_t>(dst)) {
            memcpy(dst, dst128, size);
            delete[] dst128;
        }

        return CRC;
    }

    template<bool ENCRYPT, ssize_t CRC32_LOCATION=-1>
    std::pair<uint32_t, std::string> CTRProcess(__uint128_t counter, const std::string &src)
    {
        auto dst = new __uint128_t[nrItems<__uint128_t>(src.size())];

        uint32_t CRC = CTRProcess<ENCRYPT, CRC32_LOCATION>(counter, dst, src.data(), src.size());

        auto dstString = std::string(reinterpret_cast<const char *>(dst), src.size());
        delete[] dst;

        return std::pair<uint32_t, std::string>(CRC, dstString);
    }
};

uint32_t AES128CompilerTest(__uint128_t key, __uint128_t counter, __uint128_t *buffer, size_t buffer_size)
{
    auto C = AES128(key);

    auto CRC = C.CTRProcess<true>(counter, buffer, buffer, buffer_size);

    return CRC;
}



};};
