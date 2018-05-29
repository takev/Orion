

#include <immintrin.h>
#include <boost/endian/conversion.hpp>
#include "SHA512.hpp"

namespace Orion {
namespace Rigel {

// SHA512 h: 0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1, 
//           0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
// Reversed because BigInt is in little-endian.
const BigInt<512> SHA512_initial_state = BigInt<512>("0x6a09e667f3bcc908 bb67ae8584caa73b 3c6ef372fe94f82b a54ff53a5f1d36f1 510e527fade682d1 9b05688c2b3e6c1f 1f83d9abfb41bd6b 5be0cd19137e2179");

const uint64_t SHA512_k[80] = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538, 
    0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242, 0x12835b0145706fbe, 
    0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 
    0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65, 
    0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5, 0x983e5152ee66dfab, 
    0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725, 
    0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 
    0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b, 
    0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218, 
    0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8, 0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 
    0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 
    0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec, 
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b, 0xca273eceea26619c, 
    0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6, 
    0x113f9804bef90dae, 0x1b710b35131c471b, 0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 
    0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

static inline uint64_t rotr(uint64_t x, int shift)
{
    return (x >> shift) | (x << (64 - shift));
}

static inline void SHA512CreateMessageSchedule(uint64_t *w, const uint64_t *chunk)
{
    for (int i = 0; i < 16; i++) {
        w[i] = boost::endian::big_to_native(chunk[i]);
    }

    for (int i = 16; i < 80; i++) {
        uint64_t s0 = rotr(w[i-15], 1) ^ rotr(w[i-15], 8) ^ (w[i-15] >> 7);
        uint64_t s1 = rotr(w[i-2], 19) ^ rotr(w[i-2], 61) ^ (w[i-2] >> 6);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
}

static inline void SHA512ProcessChunk(BigInt<512> &state, const uint64_t *chunk)
{
    uint64_t w[80];
    SHA512CreateMessageSchedule(w, chunk);

    // BigInt is encoded as little endian, but SHA-512 expects big endian.
    uint64_t a = state.digits[7];
    uint64_t b = state.digits[6];
    uint64_t c = state.digits[5];
    uint64_t d = state.digits[4];
    uint64_t e = state.digits[3];
    uint64_t f = state.digits[2];
    uint64_t g = state.digits[1];
    uint64_t h = state.digits[0];

    for (int i = 0; i < 80; i++) {
        uint64_t S1 = rotr(e, 14) ^ rotr(e, 18) ^ rotr(e, 41);
        uint64_t ch = (e & f) ^ (~e & g);
        uint64_t temp1 = h + S1 + ch + SHA512_k[i] + w[i];
        uint64_t S0 = rotr(a, 28) ^ rotr(a, 34) ^ rotr(a, 39);
        uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint64_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state.digits[7] += a;
    state.digits[6] += b;
    state.digits[5] += c;
    state.digits[4] += d;
    state.digits[3] += e;
    state.digits[2] += f;
    state.digits[1] += g;
    state.digits[0] += h;
}

SHA512::SHA512(void) :
    state(SHA512_initial_state), dataSize(0), overflowBufferSize(0)
{
}

SHA512::SHA512(const char *buffer, size_t bufferSize) :
    state(SHA512_initial_state), dataSize(0), overflowBufferSize(0)
{
    SHA512();
    add(buffer, bufferSize);
}

SHA512::SHA512(const std::string &str) :
    state(SHA512_initial_state), dataSize(0), overflowBufferSize(0)
{
    SHA512();
    add(str);
}

/** Add some data.
 *
 * @param buffer The bytes to hash.
 * @param bufferSize The amount of bytes to hash.
*/
void SHA512::add(const char *buffer, size_t bufferSize)
{
    dataSize += bufferSize;

    uint64_t chunk64[16];
    char *chunk = reinterpret_cast<char *>(chunk64);

    // Not enough in the buffer to process a whole chunk.
    if (overflowBufferSize + bufferSize < 128) {
        memcpy(&overflowBuffer[overflowBufferSize], buffer, bufferSize);
        overflowBufferSize += bufferSize;
        return;
    }

    // Handle the first chunk, there is at least one, because of the previous check.
    size_t firstPartSize = 128 - overflowBufferSize;
    memcpy(chunk, overflowBuffer, overflowBufferSize);
    memcpy(&chunk[overflowBufferSize], buffer, firstPartSize);
    SHA512ProcessChunk(state, chunk64);

    size_t todo = bufferSize - firstPartSize;
    size_t done = firstPartSize;

    // Handle all the full chunks.
    while (todo >= 128) {
        memcpy(chunk, &buffer[done], 128);
        SHA512ProcessChunk(state, chunk64);

        done += 128;
        todo -= 128;
    }

    // Save the non-full chunk, for next add() or finish().
    memcpy(overflowBuffer, &buffer[done], todo);
    overflowBufferSize = todo;
}

void SHA512::add(const std::string &str)
{
    add(str.data(), str.size());
}

/** Finish.
 * Hash the last bit of data and add padding.
 *
 * @return The 256 bits hash value.
 */
BigInt<512> SHA512::finish(void)
{
    uint64_t chunk64[16];
    char *chunk = reinterpret_cast<char *>(chunk64);

    // Copy the overflow buffer and pad.
    memcpy(chunk, overflowBuffer, overflowBufferSize);
    chunk[overflowBufferSize] = 0x80;
    memset(&chunk[overflowBufferSize+1], 0, 127 - overflowBufferSize);

    // If the bit counter doesn't fit in the padding, then process this
    // chunk.
    if ((127 - overflowBufferSize) < sizeof (__uint128_t)) {
        SHA512ProcessChunk(state, chunk64);
        memset(chunk, 0, 128);
    }

    // Add bit counter to end of chunk and process.
    __uint128_t dataSizeInBits = dataSize * 8;
    uint64_t dataSizeInBitsHigh = dataSizeInBits >> 64;
    uint64_t dataSizeInBitsLow = dataSizeInBits & 0xffffffffffffffff;

    chunk64[14] = boost::endian::native_to_big(dataSizeInBitsHigh);
    chunk64[15] = boost::endian::native_to_big(dataSizeInBitsLow);
    SHA512ProcessChunk(state, chunk64);

    return state;
}

};};
