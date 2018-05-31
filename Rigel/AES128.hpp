#pragma once

#include <cstdint>

#include <smmintrin.h>
#include <wmmintrin.h>
#include <nmmintrin.h>
#include <emmintrin.h>


namespace Orion {
namespace Rigel {

struct MMXKeyRounds {
    __m128i v[11];
};

struct MMXText {
    __m128i v[4];
};

/** Load all AES-128 key-rounds including round-0 into MMX registers.
 */
static inline MMXKeyRounds loadKeyRounds(const __uint128_t *keyRounds)
{
    MMXKeyRounds r;

    for (int round = 0; round <= 10; round++) {
        r.v[round] = _mm_load_si128(reinterpret_cast<const __m128i *>(&keyRounds[round]));
    }

    return r;
}

/** Execute all AES-128 Rounds on 4 parralel 128-bit blocks.
 *
 * @param keyRounds 11 MMX registers containing all the key-rounds.
 * @param text 4 blocks of text to encrypt. Encrypts in-place.
 */
static inline void AES128Encrypt(const MMXKeyRounds &keyRounds, MMXText &text)
{
    for (int i = 0; i < 4; i++) {
        text.v[i] = _mm_xor_si128(text.v[i], keyRounds.v[0]);
    }

    for (int round = 1; round < 10; round++) {
        for (int i = 0; i < 4; i++) {
            text.v[i] = _mm_aesenc_si128(text.v[i], keyRounds.v[round]);
        }
    }

    for (int i = 0; i < 4; i++) {
        text.v[i] = _mm_aesenclast_si128(text.v[i], keyRounds.v[10]);
    }
}

/** Encrypts 4 counter values in parralel.
 * 
 * @param keyRounds 11 MMX registers containing all the key-rounds.
 * @param counter MMX Register containing a 64-bit counter (lsb), and 64 bit nonce (msb). Counter is incremented
 *        by 64 (4 blocks of 16 bytes) in-place.
 * @return 4 MMX registers with four different (incremented by 16) counter+nonce values.
 */
static inline MMXText AES128EncryptCounter(const MMXKeyRounds &keyRounds, __m128i &counter)
{
    MMXText text;
    const auto blockSize = _mm_set_epi64x(0, sizeof(__uint128_t));

    text.v[0] = counter;
    text.v[1] = _mm_add_epi64(text.v[0], blockSize);
    text.v[2] = _mm_add_epi64(text.v[1], blockSize);
    text.v[3] = _mm_add_epi64(text.v[2], blockSize);
    counter   = _mm_add_epi64(text.v[3], blockSize);
    AES128Encrypt(keyRounds, text);

    // Return encrypted text.
    return text;
}

/** Encrypt 4 * 128-bit blocks in CTR-mode.
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
static inline uint32_t AES128CTREncryptQuadBlock(const MMXKeyRounds &keyRounds, __m128i &counter, uint32_t crc, __uint128_t *dst, const __uint128_t *src, size_t size)
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

/** Decrypt 4 * 128-bit blocks in CTR-mode.
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
static inline uint32_t AES128CTRDecryptQuadBlock(const MMXKeyRounds &keyRounds, __m128i &counter, uint32_t crc, __uint128_t *dst, const __uint128_t *src, size_t size, bool clearLast32BitsOfFirstBlock=false)
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
        auto tmp1 = _mm_load_si128(reinterpret_cast<__m128i *>(&key));
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[0]), tmp1);

        auto tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x01);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[1]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x02);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[2]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x04);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[3]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x08);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[4]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x10);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[5]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x20);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[6]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x40);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[7]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x80);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[8]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x1B);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[9]), tmp1);

        tmp2 = _mm_aeskeygenassist_si128(tmp1, 0x36);
        tmp1 = AES128KeyGenPart2(tmp1, tmp2);
        _mm_store_si128(reinterpret_cast<__m128i *>(&keyRounds[10]), tmp1);
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
        MMXKeyRounds kr = loadKeyRounds(keyRounds);
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));
        size_t todo = size;
        size_t blockNr = 0;
        uint32_t crc = 0xffffffff;

        while (todo >= 64) {
            crc = AES128CTREncryptQuadBlock(kr, counter, crc, &dst[blockNr], &src[blockNr], 64);
            todo-= 64;
            blockNr+= 4;
        }

        crc = AES128CTREncryptQuadBlock(kr, counter, crc, &dst[blockNr], &src[blockNr], todo);

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
        MMXKeyRounds kr = loadKeyRounds(keyRounds);
        auto counter = _mm_load_si128(reinterpret_cast<__m128i *>(&_counter));
        size_t todo = size;
        size_t blockNr = 0;
        uint32_t crc = 0xffffffff;

        if (todo >= 64) {
            crc = AES128CTRDecryptQuadBlock(kr, counter, crc, &dst[blockNr], &src[blockNr], 64, clearLast32BitsOfFirstBlock);
            todo-= 64;
            blockNr+= 4;

            while (todo >= 64) {
                crc = AES128CTRDecryptQuadBlock(kr, counter, crc, &dst[blockNr], &src[blockNr], 64);
                todo-= 64;
                blockNr+= 4;
            }

            crc = AES128CTRDecryptQuadBlock(kr, counter, crc, &dst[blockNr], &src[blockNr], todo);

        } else {
            crc = AES128CTRDecryptQuadBlock(kr, counter, crc, &dst[blockNr], &src[blockNr], todo, clearLast32BitsOfFirstBlock);
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
