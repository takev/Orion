#pragma once

#include <cstdint>

#include <smmintrin.h>
#include <wmmintrin.h>
#include <nmmintrin.h>
#include <emmintrin.h>


namespace Orion {
namespace Rigel {

struct MMXText {
    __m128i v[8];

    static size_t nrBlocks(void) {
        return sizeof(v) / sizeof(*v);
    }
};

/** Encrypts 4 counter values in parralel.
 * 
 * @param keyRounds 11 MMX registers containing all the key-rounds.
 * @param counter MMX Register containing a 64-bit counter (lsb), and 64 bit nonce (msb). Counter is incremented
 *        by 64 (4 blocks of 16 bytes) in-place.
 * @return 4 MMX registers with four different (incremented by 16) counter+nonce values.
 */
static inline MMXText AES128EncryptCounter(const __uint128_t *keyRounds, __m128i &counter)
{
    MMXText cypher;

    // Round 0
    {
        const auto blockSize = _mm_set_epi64x(0, sizeof(__uint128_t));
        auto key = _mm_load_si128(reinterpret_cast<const __m128i *>(keyRounds[0]));
        cypher.v[0] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        cypher.v[1] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        cypher.v[2] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        cypher.v[3] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        cypher.v[4] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        cypher.v[5] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        cypher.v[6] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
        cypher.v[7] = _mm_xor_si128(counter, key);
        counter = _mm_add_epi64(counter, blockSize);
    }

#define AES128_ROUND(instruction, x)\
    {\
        auto key = _mm_load_si128(reinterpret_cast<const __m128i *>(keyRounds[x]));\
        cypher.v[0] = instruction(cypher.v[0], key);\
        cypher.v[1] = instruction(cypher.v[1], key);\
        cypher.v[2] = instruction(cypher.v[2], key);\
        cypher.v[3] = instruction(cypher.v[3], key);\
        cypher.v[4] = instruction(cypher.v[4], key);\
        cypher.v[5] = instruction(cypher.v[5], key);\
        cypher.v[6] = instruction(cypher.v[6], key);\
        cypher.v[7] = instruction(cypher.v[7], key);\
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

    // Return encrypted counter.
    return cypher;
}

/** Encrypt 128-bit blocks in CTR-mode.
 *
 * @param keyRounds 11 MMX Registers holding the AES key-rounds.
 * @param counter 1 MMX Register containing nonce+counter value, where the lower 64 bits is incremented/rolled-over,
 *        the higher 64 bits remain untouched.
 * @param crc The crc value from previous processing.
 * @param dst Destination buffer with the encrypted data. Src and dst may alias.
 * @param src Source buffer with the plain-text data. Src and dst may alias.
 * @param size Number of bytes in dst and src, does not need to be a multiple of 16 bytes.
 * @return CRC-32C of the plain-text data.
 */
static inline uint32_t AES128CTREncryptBlocks(const __uint128_t *keyRounds, __m128i &counter, uint32_t crc, __uint128_t *dst, const __uint128_t *src, size_t size)
{
    auto cypherText = AES128EncryptCounter(keyRounds, counter);

    auto nrBlocks = size / sizeof(__uint128_t);
    auto nrLastBytes = size % sizeof(__uint128_t);

    size_t blockNr;
    for (blockNr = 0; blockNr < nrBlocks; blockNr++) {
        auto text = _mm_load_si128(reinterpret_cast<const __m128i *>(&src[blockNr]));
        auto textLo = _mm_cvtsi128_si64(text);
        auto textHi = _mm_extract_epi64(text, 1);

        crc = static_cast<uint32_t>(_mm_crc32_u64(static_cast<uint64_t>(crc), textLo));
        crc = static_cast<uint32_t>(_mm_crc32_u64(static_cast<uint64_t>(crc), textHi));

        text = _mm_xor_si128(text, cypherText.v[blockNr]);
        _mm_store_si128(reinterpret_cast<__m128i *>(&dst[blockNr]), text);
    }

    if (nrLastBytes) {
        __uint128_t cypherTextBuffer128;
        auto cypherTextBuffer8 = reinterpret_cast<uint8_t *>(&cypherTextBuffer128);
        auto srcByteBuffer = reinterpret_cast<const uint8_t *>(&src[blockNr]);
        auto dstByteBuffer = reinterpret_cast<uint8_t *>(&dst[blockNr]);

        _mm_store_si128(reinterpret_cast<__m128i *>(&cypherTextBuffer128), cypherText.v[blockNr]);

        for (size_t byteNr = 0; byteNr < nrLastBytes; byteNr++) {
            auto byte = srcByteBuffer[byteNr];
            crc = _mm_crc32_u8(crc, byte);
            byte ^= cypherTextBuffer8[byteNr];
            dstByteBuffer[byteNr] = byte;
        }
    }

    return crc;
}

/** Decrypt 128-bit blocks in CTR-mode.
 *
 * @param keyRounds 11 MMX Registers holding the AES key-rounds.
 * @param counter 1 MMX Register containing nonce+counter value, where the lower 64 bits is incremented/rolled-over,
 *        the higher 64 bits remain untouched.
 * @param crc The crc value from previous processing.
 * @param dst Destination buffer with the plain-text data. Src and dst may alias.
 * @param src Source buffer with the encrypted data. Src and dst may alias.
 * @param size Number of bytes in dst and src, does not need to be a multiple of 16 bytes.
 * @param clearLast32BitsOfFirstBlock For use when the CRC was encrypted within the data, and need to be cleared.
 * @return CRC-32C of the plain-text data.
 */
static inline uint32_t AES128CTRDecryptBlocks(const __uint128_t *keyRounds, __m128i &counter, uint32_t crc, __uint128_t *dst, const __uint128_t *src, size_t size, bool clearLast32BitsOfFirstBlock=false)
{
    auto cypherText = AES128EncryptCounter(keyRounds, counter);

    auto nrBlocks = size / sizeof(__uint128_t);
    auto nrLastBytes = size % sizeof(__uint128_t);

    size_t blockNr;
    for (blockNr = 0; blockNr < nrBlocks; blockNr++) {
        auto text = _mm_load_si128(reinterpret_cast<const __m128i *>(&src[blockNr]));
        text = _mm_xor_si128(text, cypherText.v[blockNr]);
        _mm_store_si128(reinterpret_cast<__m128i *>(&dst[blockNr]), text);

        auto textLo = _mm_cvtsi128_si64(text);
        auto textHi = _mm_extract_epi64(text, 1);

        // Clear the encrypted CRC value.
        if (clearLast32BitsOfFirstBlock && blockNr == 0) {
            textHi &= 0x00000000ffffffff;
        }

        crc = static_cast<uint32_t>(_mm_crc32_u64(static_cast<uint64_t>(crc), textLo));
        crc = static_cast<uint32_t>(_mm_crc32_u64(static_cast<uint64_t>(crc), textHi));
    }

    if (nrLastBytes) {
        __uint128_t cypherTextBuffer128;
        auto cypherTextBuffer8 = reinterpret_cast<uint8_t *>(&cypherTextBuffer128);
        auto srcByteBuffer = reinterpret_cast<const uint8_t *>(&src[blockNr]);
        auto dstByteBuffer = reinterpret_cast<uint8_t *>(&dst[blockNr]);

        _mm_store_si128(reinterpret_cast<__m128i *>(&cypherTextBuffer128), cypherText.v[blockNr]);

        for (size_t byteNr = 0; byteNr < nrLastBytes; byteNr++) {
            auto byte = srcByteBuffer[byteNr];
            byte ^= cypherTextBuffer8[byteNr];
            crc = _mm_crc32_u8(crc, byte);
            dstByteBuffer[byteNr] = byte;
        }
    }

    return crc;
}

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
        uint32_t crc = 0xffffffff;

        while (todo >= sizeof (MMXText)) {
            crc = AES128CTREncryptBlocks(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], sizeof (MMXText));
            todo-= sizeof (MMXText);
            blockNr+= MMXText::nrBlocks();
        }

        crc = AES128CTREncryptBlocks(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], todo);

        return crc ^ 0xfffffff;
    }

    /** Decrypt buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the dst buffer.
     */
    uint32_t CTRDecrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size, bool clearLast32BitsOfFirstBlock=false)
    {
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));
        size_t todo = size;
        size_t blockNr = 0;
        uint32_t crc = 0xffffffff;

        if (todo >= sizeof (MMXText)) {
            crc = AES128CTRDecryptBlocks(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], sizeof (MMXText), clearLast32BitsOfFirstBlock);
            todo-= sizeof (MMXText);
            blockNr+= MMXText::nrBlocks();

            while (todo >= sizeof (MMXText)) {
                crc = AES128CTRDecryptBlocks(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], sizeof (MMXText));
                todo-= sizeof (MMXText);
                blockNr+= MMXText::nrBlocks();
            }

            crc = AES128CTRDecryptBlocks(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], todo);

        } else {
            crc = AES128CTRDecryptBlocks(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], todo, clearLast32BitsOfFirstBlock);
        }

        return crc ^ 0xffffffff;
    }
};

uint32_t AES128CompilerTest(__uint128_t key, __uint128_t counter, __uint128_t *buffer, size_t buffer_size)
{
    auto C = AES128(key);

    auto crc = C.CTREncrypt(counter, buffer, buffer, buffer_size);

    return crc;
}



};};
