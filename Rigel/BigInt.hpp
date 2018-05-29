#include <xmmintrin.h>
#include <immintrin.h>
#include <cassert>
#include <ostream>

#include "utils.hpp"
#include "int_utils.hpp"
#include "string_utils.hpp"

namespace Orion {
namespace Rigel {
namespace BigInt {
#ifdef NR_DIGITS
#error "NR_DIGITS macro is already defined"
#endif
#define NR_DIGITS(x) ((x+63)/64)

struct bigint_error: virtual boost::exception {};
struct bigint_random_error: virtual bigint_error, virtual std::exception {};
struct bigint_character_error: virtual bigint_error, virtual std::exception {};
struct bigint_overflow_error: virtual bigint_error, virtual std::exception {};
struct bigint_barret_error: virtual bigint_error, virtual std::exception {};

template<int K> struct BarretReduction;

/** Unsigned big integer.
 * This class is used for cryptographic calculations, such as Diffie Hellman Key Exchange.
 *
 * For this reason all function that work with BigInt need to run at constant time
 * independed of the integer values, to reduce the change for timing based side-channel attacks.
 */
template<int N>
struct BigInt {
    uint64_t digits[NR_DIGITS(N)];

    BigInt(void) = default;

    /** Explicit conversion for static_cast();
     */
    template<int O>
    explicit inline BigInt(BigInt<O> &other) {
        for (int i = 0; i < NR_DIGITS(N); i++) {
            digits[i] = (i < NR_DIGITS(O)) ? other.digits[i] : 0;
        }
    }

    explicit inline BigInt(const std::string &str) {
        if (startswith(str, "0x") || startswith(str, "0X")) {
            initHexString(str, 2);
        } else if (startswith(str, "0d") || startswith(str, "0D")) {
            initDecString(str, 2);
        } else {
            initDecString(str, 0);
        }
    }

    explicit inline BigInt(uint64_t other) {
        digits[0] = other;
        memset(&digits[1], 0, (NR_DIGITS(N) - 1) * sizeof (uint64_t));
    }

    inline size_t size(void) const {
        return NR_DIGITS(N) * sizeof (uint64_t);
    }

    inline const char *data(void) const {
        return reinterpret_cast<char *>(&digits[0]);
    }

    inline void toLittleEndian(void) {
        for (int i = 0; i < NR_DIGITS(N); i++) {
            digits[i] = boost::endian::native_to_little(digits[i]);
        }
    }

    inline bool getBit(int i) const {
        uint64_t d = digits[i / 64];
        return static_cast<bool>((d >> (i % 64)) & 1);
    }

    inline void setBit(int i, bool x) {
        uint64_t d = static_cast<uint64_t>(x) << (i % 64);
        digits[i / 64] |= d;
    }

    template<int O>
    inline int compare(const BigInt<O> &other) const {
        int r = 0;

        for (int i = std::max(NR_DIGITS(N), NR_DIGITS(O)) - 1; i >= 0; i--) {
            uint64_t digit_t = (i < NR_DIGITS(N)) ? (*this).digits[i] : 0;
            uint64_t digit_o = (i < NR_DIGITS(O)) ? other.digits[i] : 0;

            r = (r == 0) & (digit_t < digit_o) ? -1 : r;
            r = (r == 0) & (digit_t > digit_o) ?  1 : r;
        }

        return r;
    }

    /** Accumulate a multiplication of two big integers.
     *
     * @param a value
     * @param b value
     * @return true if the subtraction underflows the ressult.
     */
    template<int A_SIZE, int B_SIZE>
    inline bool fma(const BigInt<A_SIZE> &a, const BigInt<B_SIZE> &b) {
        bool overflow = false;
        for (int b_i = 0; b_i < NR_DIGITS(B_SIZE); b_i++) {
            int r_i = b_i;
            uint64_t carry = 0;

            for (int a_i = 0; a_i < NR_DIGITS(A_SIZE); a_i++) {
                // Multiplying two 64 bit integers and adding two 64 bit integers.
                // Will fit exactly in a single 128 bit number.
                intel_intrinsic_uint64 hi;
                intel_intrinsic_uint64 lo = _mulx_u64(a.digits[a_i], b.digits[b_i], &hi);
                hi += _addcarryx_u64(0, lo, carry, &lo);

                intel_intrinsic_uint64 r_digit = (r_i < NR_DIGITS(N)) ? digits[r_i] : 0;
                hi += _addcarryx_u64(0, lo, r_digit, &r_digit);

                if (r_i < NR_DIGITS(N)) {
                    digits[r_i] = r_digit;
                } else {
                    overflow |= r_digit > 0;
                }

                // Get ready for next iteration.
                r_i++;
                carry = hi;
            }

            // Carry through until the end of the result. Since the result
            // can already have a value to begin with.
            for (; r_i < NR_DIGITS(N); r_i++) {
                carry = _addcarryx_u64(0, carry, digits[r_i], reinterpret_cast<intel_intrinsic_uint64 *>(&digits[r_i]));
            }

            // Any carry left over at the end is an overflow.
            overflow |= carry > 0;
        }

        return overflow;
    }

    /** Shift left by at most 1 digit.
     *
     * @param count The amount to shift between 0 and 63.
     * @param carry The value to shift into the left.
     * @return overflow
     */
    inline bool partialShiftLeft(int count, uint64_t carry=0) {
        assert(count >= 0 && count <= 63);

        for (int i = 0; i < NR_DIGITS(N); i++) {
            uint64_t d = digits[i];
            
            digits[i] = (d << count) | carry;
            carry = d >> (64 - count);
        }

        return carry > 0;
    }

    /** Shift right by at most 1 digit.
     *
     * @param count The amount to shift between 0 and 63.
     * @param carry The value to shift into the left.
     * @return overflow
     */
    inline bool partialShiftRight(int count, uint64_t carry=0) {
        assert(count >= 0 && count <= 63);

        for (int i = NR_DIGITS(N) - 1; i >= 0; i--) {
            uint64_t d = digits[i];
            
            digits[i] = (d >> count) | carry;
            carry = d << (64 - count);
        }

        return carry > 0;
    }

    inline bool partialMultiply(uint64_t multiplier, uint64_t carry=0) {
        for (int i = 0; i < NR_DIGITS(N); i++) {
            // Multiplying two 64 bit integers and adding two 64 bit integers.
            // Will fit exactly in a single 128 bit number.
            intel_intrinsic_uint64 hi;
            intel_intrinsic_uint64 lo = _mulx_u64(digits[i], multiplier, &hi);
            hi += _addcarryx_u64(0, lo, carry, &lo);

            digits[i] = lo;
            carry = hi;
        }
        return carry > 0;
    }

    inline bool digitShiftLeft(int count) {
        bool overflow = false;

        // Check for overflow.
        for (int i = NR_DIGITS(N) - 1; i >= NR_DIGITS(N) - count; i++) {
            overflow |= digits[i] > 0;
        }

        for (int i = NR_DIGITS(N) - 1; i >= 0; i--) {
            digits[i] = i >= count ? digits[i - count] : 0;
        }

        return overflow;
    }

    inline bool digitShiftRight(int count) {
        bool overflow = false;

        // Check for overflow.
        for (int i = 0; i < count; i++) {
            overflow |= digits[i] > 0;
        }

        for (int i = count; i < NR_DIGITS(N) + count; i++) {
            digits[i - count] = i < NR_DIGITS(N) ? digits[i] : 0;
        }

        return overflow;
    }

    /** Initialize by filling the integer with random data.
     */
    inline void initRandom(void) {
        for (int i = 0; i < NR_DIGITS(N); i++) {
            int retry_count = 10;
            intel_intrinsic_uint64 random_digit;

        retry:
            if (unlikely(!_rdrand64_step(&random_digit))) {
                if (retry_count-- == 0) {
                    BOOST_THROW_EXCEPTION(bigint_random_error());
                }

                // Give other hyperthreads some time to run.
                _mm_pause();
                goto retry;
            }

            digits[i] = random_digit;
        }
    }

    /** Initialize by adding two big integers together.
     *
     * @param a value
     * @param b value
     * @return true if the addition overflows the ressult.
     */
    template<int A_SIZE, int B_SIZE>
    inline bool initAdd(const BigInt<A_SIZE> &a, const BigInt<B_SIZE> &b) {
        char carry = 0;
        for (int i = 0; i < NR_DIGITS(N); i++) {
            uint64_t a_digit = (i < NR_DIGITS(A_SIZE)) ? a.digits[i] : 0;
            uint64_t b_digit = (i < NR_DIGITS(B_SIZE)) ? b.digits[i] : 0;

            carry = _addcarryx_u64(carry, a_digit, b_digit, reinterpret_cast<intel_intrinsic_uint64 *>(&digits[i]));
        }

        // Check for overflow. Should be optimized when not checking the return value.
        bool overflow = carry;
        for (int i = NR_DIGITS(N); i < NR_DIGITS(A_SIZE); i++) {
            overflow |= a.digits[i] > 0;
        }
        for (int i = NR_DIGITS(N); i < NR_DIGITS(B_SIZE); i++) {
            overflow |= b.digits[i] > 0;
        }

        return overflow;
    }

    /** Initialize by subtracting two big integers from each other.
     *
     * @param a value
     * @param b value
     * @return true if the subtraction underflows the ressult.
     */
    template<int A_SIZE, int B_SIZE>
    inline bool initSubtract(const BigInt<A_SIZE> &a, const BigInt<B_SIZE> &b) {
        bool underflow = false;
        char borrow = 0;
        int i;
        for (i = 0; i < std::max(NR_DIGITS(A_SIZE), NR_DIGITS(B_SIZE)); i++) {
            uint64_t a_digit = (i < NR_DIGITS(A_SIZE)) ? a.digits[i] : 0;
            uint64_t b_digit = (i < NR_DIGITS(B_SIZE)) ? b.digits[i] : 0;
            intel_intrinsic_uint64 r_digit;

            borrow = fixed_subborrow_u64(borrow, a_digit, b_digit, &r_digit);

            if (i < NR_DIGITS(N)) {
                digits[i] = r_digit;
            } else {
                underflow |= r_digit > 0;
            }
        }

        for (; i < NR_DIGITS(N); i++) {
            digits[i] = 0;
        }

        return underflow | (borrow != 0);
    }

    /** Initialize by dividing two integers.
     *
     * @param a value
     * @param b value
     * @return Remainder value.
     */
    template<int A, int B>
    inline BigInt<B> initDivision(const BigInt<A> &a, const BigInt<B> &b) {
        memset(digits, 0, NR_DIGITS(N) * sizeof (uint64_t));
        auto remainder = BigInt<B+1>(0);
        auto zero = BigInt<B>(0);

        for (int i = A - 1; i >= 0; i--) {
            remainder.partialShiftLeft(1, a.getBit(i));

            auto remainderLEb = remainder >= b;
            remainder -= remainderLEb ? b : zero;
            setBit(i, remainderLEb);
        }

        return BigInt<B>(remainder);
    }

    inline BigInt<N> modularPower(const BigInt<N> &exponent, const BarretReduction<N> &br) {
        auto one = BigInt<N>(1);
        auto result = BigInt<N>(1);

        auto base = br.modulo(*this);
        for (int i = 0; i < N; i++) {
            auto product = result * (exponent.getBit(i) ? base : one);

            result = br.modulo(product);
            base = br.modulo(base * base);
        }

        return result;
    }

    inline BigInt<N> modularPower(const BigInt<N> &exponent, const BigInt<N> &modulus) {
        auto br = BarretReduction<N>(modulus);
        return modularPower(exponent, br);
    }

    inline void initHexString(const std::string &str, size_t offset) {
        memset(digits, 0, NR_DIGITS(N) * sizeof (uint64_t));

        for (; offset < str.size(); offset++) {
            char nibble;

            switch (auto c = str[offset]) {
            case '0' ... '9': nibble = c - '0'; break;
            case 'a' ... 'f': nibble = c - 'a' + 10; break;
            case 'A' ... 'F': nibble = c - 'A' + 10; break;
            case ' ': case '\t': case '\n': case '\r': case ',': case '\'': continue;
            default: BOOST_THROW_EXCEPTION(bigint_character_error());
            }

            if (partialShiftLeft(4, nibble)) {
                BOOST_THROW_EXCEPTION(bigint_overflow_error());
            }
        }
    }

    inline void initDecString(const std::string &str, size_t offset) {
        memset(digits, 0, NR_DIGITS(N) * sizeof (uint64_t));

        for (; offset < str.size(); offset++) {
            char nibble;
            switch (auto c = str[offset]) {
            case '0' ... '9': nibble = c - '0'; break;
            case ' ': case '\t': case '\n': case '\r': case ',': case '\'': continue;
            default: BOOST_THROW_EXCEPTION(bigint_character_error());
            }

            if (partialMultiply(10, nibble)) {
                BOOST_THROW_EXCEPTION(bigint_overflow_error());
            }
        }
    }

    template<int O>
    inline bool operator==(const BigInt<O> &other) const {
        return compare(other) == 0;
    }

    template<int O>
    inline bool operator!=(const BigInt<O> &other) const {
        return compare(other) != 0;
    }

    template<int O>
    inline bool operator>=(const BigInt<O> &other) const {
        return compare(other) >= 0;
    }

    template<int O>
    inline bool operator>(const BigInt<O> &other) const {
        return compare(other) > 0;
    }

    template<int O>
    inline bool operator<=(const BigInt<O> &other) const {
        return compare(other) <= 0;
    }

    template<int O>
    inline bool operator<(const BigInt<O> &other) const {
        return compare(other) < 0;
    }

    inline BigInt<N> &operator<<=(int count) {
        if (count / 64) {
            digitShiftLeft(count / 64);
        }
        if (count % 64) {
            partialShiftLeft(count % 64, 0);
        }
        return *this;
    }

    inline BigInt<N> operator<<(int count) const {
        auto r = *this;
        r <<= count;
        return r;
    }

    inline BigInt<N> &operator>>=(int count) {
        if (count / 64) {
            digitShiftRight(count / 64);
        }
        if (count % 64) {
            partialShiftRight(count % 64, 0);
        }
        return *this;
    }

    inline BigInt<N> operator>>(int count) const {
        auto r = *this;
        r >>= count;
        return r;
    }

    template<int O>
    inline BigInt<N> &operator+=(const BigInt<O> &other) {
        initAdd(*this, other);
        return *this;
    }

    template<int O>
    inline BigInt<std::max(N, O)+1> operator+(const BigInt<O> &other) const {
        BigInt<std::max(N, O)+1> r;
        r.initAdd(*this, other);
        return r;
    }

    template<int O>
    inline BigInt<N> &operator-=(const BigInt<O> &other) {
        initSubtract(*this, other);
        return *this;
    }

    template<int O>
    inline BigInt<std::max(N, O)> operator-(const BigInt<O> &other) const {
        BigInt<std::max(N, O)> r;
        r.initSubtract(*this, other);
        return r;
    }

    template<int O>
    inline BigInt<N> operator/(const BigInt<O> &other) const {
        BigInt<N> r;
        r.initDivision(*this, other);
        return r;
    }

    template<int O>
    inline BigInt<N> &operator/=(const BigInt<O> &other) {
        auto r = *this;
        initDivision(r, other);
        return *this;
    }

    template<int O>
    inline BigInt<O> operator%(const BigInt<O> &other) const {
        BigInt<N> r;
        return r.initDivision(*this, other);
    }

    template<int O>
    inline BigInt<N> &operator%=(const BigInt<O> &other) {
        auto r = *this;
        BigInt<N> dummy;
        BigInt<N> remainder;

        remainder = dummy.initDivision(r, other);

        memcpy(digits, remainder.digits, N * sizeof (uint64_t));
        return *this;
    }

    template<int O>
    inline BigInt<N+O> operator*(const BigInt<O> &other) const {
        auto r = BigInt<N+O>(0);
        r.fma(*this, other);
        return r;
    }

    template<int O>
    friend std::ostream& operator<<(std::ostream &os, const BigInt<O> x);
};

template<int N>
inline std::ostream& operator<<(std::ostream& os, const BigInt<N> x)
{
    os << "0x";

    bool found_non_zero = false;
    for (int i = NR_DIGITS(N) - 1; i >= 0; i--) {
        for (int j = 60; j >= 0; j -= 4) {
            auto nible = (x.digits[i] >> j) & 0xf;

            if (found_non_zero or nible > 0) {
                found_non_zero = true;

                if (nible < 10) {
                    os << static_cast<char>(nible + '0');
                } else {
                    os << static_cast<char>(nible - 10 + 'a');
                }
            }
        }
    }

    if (!found_non_zero) {
        os << '0';
    }

    return os;
}

template<int N>
inline BigInt<N> BigIntRandom(void) {
    BigInt<N> r;
    r.initRandom();
    return r;
}

template<int N>
inline BigInt<N> BigIntHighbit(void) {
    auto r = BigInt<N>(0);
    r.setBit(N-1, true);
    return r;
}

/** Fast division using Barret Reduction.
 * https://en.wikipedia.org/wiki/Barrett_reduction
 *
 * The following algorithm that is implemented is constant time:
 * ```
 *     r = 4**K / modulus
 *     quotient = (x * r) / (4**K)
 *     remainder = x - (quotient * modulus)
 *
 *     if (remainder >= modulus) {
 *         remainder -= modulus;
 *         quotient -= 1;
 *     }
 * ```
 */
template<int K>
struct BarretReduction {
    BigInt<K> modulus;
    BigInt<2*K> r;

    BarretReduction(const BigInt<K> &modulus) :
        modulus(modulus), r()
    {
        auto scaledOne = BigIntHighbit<2*K+1>();
        r.initDivision(scaledOne, modulus);
    }

    template<int X>
    inline BigInt<X> divide(BigInt<K> &remainder, const BigInt<X> &x) const {
        auto longQuotient = x * r;
        longQuotient >>= 2*K;

        auto quotient = BigInt<X>(longQuotient);

        auto near_x = quotient * modulus;
        remainder.initSubtract(x, near_x);

        // Adjust the remainder if the quotient was off by one.
        // Do this by always executing the adjustment, for constant cpu time.
        bool offByOne = remainder >= modulus;
        remainder -= offByOne ? modulus      : BigInt<K>(0);
        quotient  -= offByOne ? BigInt<X>(1) : BigInt<X>(0);

        return quotient;
    }

    template<int X>
    inline BigInt<X> divide(const BigInt<X> &x) const {
        BigInt<K> remainder;
        return divide(remainder, x);
    }

    template<int X>
    inline BigInt<K> modulo(const BigInt<X> &x) const {
        BigInt<K> remainder;
        divide(remainder, x);
        return remainder;
    }
};

#undef NR_DIGITS
};};};
