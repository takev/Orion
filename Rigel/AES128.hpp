#pragma once

#include <BigInt.h>

namespace Orion {
namespace Rigel {

const uint64_t AES_RCON[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36
}

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
        r.v[round] = _mm_load_si128(keyRounds[round]);
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
        text.v[i] = _mm_xor_si128(keyRounds[0]);
    }

    for (int round = 1; round < 10; round++) {
        for (int i = 0; i < 4; i++) {
            text.v[i] = _mm_aesenc_si128(keyRounds[round]);
        }
    }

    for (int i = 0; i < 4; i++) {
        text.v[i] = _mm_aesenclast_si128(keyRounds[round]);
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
    const auto blockSize = _mm_load_epi64(0, sizeof(__uint128_t));

    text.v[0] = counter;
    text.v[1] = _mm_add_epi64(ctrs.v[0], blockSize);
    text.v[2] = _mm_add_epi64(ctrs.v[1], blockSize);
    text.v[3] = _mm_add_epi64(ctrs.v[2], blockSize);
    counter   = _mm_add_epi64(ctrs.v[3], blockSize);
    AES128Encrypt(kr, ctrs);

    return ctrs;
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
        auto text = _mm_load_si128(src[blockNr]);
        auto textLo = _mm_cvtsi128_si64(text);
        auto textHi = _mm_extract_epi64(text, 1);

        crc = static_cast<uint32_t>(_mm_crc32_u64(static_cast<uint64_t>(crc), text_lo));
        crc = static_cast<uint32_t>(_mm_crc32_u64(static_cast<uint64_t>(crc), text_hi));

        text = _mm_xor_si128(text, cypherText.v[blockNr]);
        _mm_store_si128(&dst[blockNr], text);
    }

    if (nrLastBytes) {
        auto srcByteBuffer = reinterpret_cast<uint8_t *>(&src[blockNr]);
        auto dstByteBuffer = reinterpret_cast<uint8_t *>(&dst[blockNr]);

        for (byteNr = 0; byteNr < nrLastBytes; byteNr++) {
            auto byte = srcByteBuffer[byteNr];
            crc = _mm_crc32_u8(crc, byte);
            byte ^= _mm_extract_epi8(cypherText.v[blockNr], byteNr);
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
        auto text = _mm_load_si128(&src[blockNr]);
        text = _mm_xor_si128(text, cypherText.v[blockNr]);
        _mm_store_si128(&dst[blockNr], text);

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
        auto srcByteBuffer = reinterpret_cast<uint8_t *>(&src[blockNr]);
        auto dstByteBuffer = reinterpret_cast<uint8_t *>(&dst[blockNr]);

        for (byteNr = 0; byteNr < nrLastBytes; byteNr++) {
            auto byte = srcByteBuffer[byteNr];
            byte ^= _mm_extract_epi8(cypherText.v[blockNr], byteNr);
            crc = _mm_crc32_u8(crc, byte);
            dstByteBuffer[byteNr] = byte;
        }
    }

    return crc;
}

class AES128 {
    __uint128_t key;
    __uint128_t roundKeys[10];    

    AES128::AES128(__uint128_t key) :
        key(key)
    {
        auto tmp1 = _mm_load_si128(&key);
        _mm_store(&keyRounds[0], tmp1);
        for (int round = 1; round <= 10; round++) {
            auto tmp2 = _mm_aeskeygenassist_si128(tmp1, AES_RCON[round]);

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

            _mm_store(&keyRounds[round], tmp1);
        }
    }

    /** Encrypt buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the src buffer.
     */
    inline uint32_t AES128::CTREncrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size)
    {
        MMXKeyRounds kr = loadKeyRounds(keyRounds);
        auto counter = _mm_load_si128(&_counter);
        size_t todo = size;
        size_t blockNr = 0;
        uint32_t crc = 0xffffffff;

        while (todo >= 64) {
            crc = AES128CTREncryptQuadBlock(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], 64);
            todo-= 64;
            blockNr+= 4;
        }

        crc = AES128CTREncryptQuadBlock(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], todo);

        return crc ^ 0xfffffff;
    }

    /** Encrypt buffer in-place.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param buffer Buffer is modified in-place.
     * @param size Size of the buffer in bytes.
     * @return CRC-32C value of the buffer before encrypting.
     */
    inline uint32_t AES128::CTREncrypt(__uint128_t _counter, __uint128_t *buffer, size_t size)
    {
        return CTREncrypt(counter, buffer, buffer, size);
    }

    /** Decrypt buffer.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param dst Destination buffer. Dst and src may alias.
     * @param src Source buffer. Dst and src may alias.
     * @param size Size of the src and dst buffers in bytes.
     * @return CRC-32C value of the dst buffer.
     */
    uint32_t AES128::CTRDecrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size, bool clearLast32BitsOfFirstBlock=false)
    {
        MMXKeyRounds kr = loadKeyRounds(keyRounds);
        auto counter = _mm_load_si128(&_counter);
        size_t todo = size;
        size_t blockNr = 0;
        uint32_t crc = 0xffffffff;

        if (todo >= 64) {
            crc = AES128CTREncryptQuadBlock(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], 64, clearLast32BitsOfFirstBlock);
            todo-= 64;
            blockNr+= 4;

            while (todo >= 64) {
                crc = AES128CTREncryptQuadBlock(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], 64);
                todo-= 64;
                blockNr+= 4;
            }

            crc = AES128CTREncryptQuadBlock(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], todo);

        } else {
            crc = AES128CTREncryptQuadBlock(keyRounds, counter, crc, &dst[blockNr], &src[blockNr], todo, clearLast32BitsOfFirstBlock);
        }

        return crc ^ 0xffffffff;
    }

    /** Decrypt buffer in-place.
     *
     * @param counter CTR counter+nonce value at start of buffer.
     * @param buffer Buffer is modified in-place.
     * @param size Size of the buffer in bytes.
     * @return CRC-32C value of the buffer after decrypting.
     */
    uint32_t AES128::CTRDecrypt(__uint128_t _counter, __uint128_t *dst, const __uint128_t *src, size_t size, bool clearLast32BitsOfFirstBlock=false)
};

};};
