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
#include <cstdint>
#include <atomic>
#include <boost/filesystem.hpp>
#include <x86intrin.h>

#include "int_utils.hpp"

namespace Orion {
namespace Rigel {

struct TimeCalibration {
    __uint128_t timeAdjust;

    inline void setGainBias(uint64_t gain, uint64_t bias)
    {
        __uint128_t newTimeAdjust = (static_cast<__uint128_t>(gain) << 64) | static_cast<__uint128_t>(bias);

        // The 128-bit CAS instruction is the only way to write 128 bits atomically.
        __uint128_t oldTimeAdjust;
        do {
            oldTimeAdjust = __sync_val_compare_and_swap(&timeAdjust, 0, 0);
        } while (!__sync_bool_compare_and_swap(&timeAdjust, oldTimeAdjust, newTimeAdjust));
    }

    /** Convert a host tick to time.
     * This function is useful if you need to get a low latency timestamp,
     * and can convert to actual time on a slower path.
     */
    inline std::pair<uint64_t, uint64_t> getGainBias(void)
    {
        // The 128-bit CAS instruction is the only way to read 128 bits atomically.
        // An actual swap is never executed (no cache dirtying), due to the 0 compare value.
        // This is still wait-free, because we do not need to loop.
        __uint128_t currentTimeAdjust = __sync_val_compare_and_swap(&timeAdjust, 0, 0);

        uint64_t gain = static_cast<uint64_t>(currentTimeAdjust >> 64);
        uint64_t bias = static_cast<uint64_t>(currentTimeAdjust);

        return std::pair<uint64_t, uint64_t>{gain, bias};
    }

} __attribute__ ((aligned(CACHE_LINE_SIZE)));

/** A duration in nanoseconds.
 * The duration is incremented based on atomic clocks, without
 * leap seconds or drift.
 */
struct Duration {
    int64_t intrinsic;

    Duration(void) = default;

    Duration(int64_t x) : intrinsic(x) {}

    inline int64_t toNanoseconds(void) const {
        return intrinsic;
    }

    inline int64_t toMicroseconds(void) const {
        return intrinsic / 1000;
    }

    inline int64_t toMilliseconds(void) const {
        return intrinsic / 1000000;
    }

    inline int64_t toSeconds(void) const {
        return intrinsic / 1000000000;
    }
};

/** The time in TAI nanoseconds since January 1st of the year 2000.
 * TAI is used so we do not have to acount for clock drift during
 * days with leap seconds.
 *
 * A int64_t can store up to +/- 299 years in nanoseconds.
 */
struct Time {
    int64_t intrinsic;

    Time(void) = default;

    Time(int64_t x) : intrinsic(x) {}

    inline Duration operator-(const Time &other) const {
        return Duration(intrinsic - other.intrinsic);
    }

    inline Time operator+(const Duration &other) const {
        return Time(intrinsic + other.intrinsic);
    }

    inline Time operator+(const int64_t nanoseconds) const {
        return Time(intrinsic + nanoseconds);
    }

    inline bool operator==(const Time &other) const {
        return intrinsic == other.intrinsic;
    }

    inline bool operator!=(const Time &other) const {
        return intrinsic != other.intrinsic;
    }

    inline bool operator<(const Time &other) const {
        return intrinsic < other.intrinsic;
    }

    inline bool operator>(const Time &other) const {
        return intrinsic > other.intrinsic;
    }

    inline bool operator<=(const Time &other) const {
        return intrinsic <= other.intrinsic;
    }

    inline bool operator>=(const Time &other) const {
        return intrinsic <= other.intrinsic;
    }

    inline Time operator++(int) {
        intrinsic++;
        return *this;
    }

};

const Time DISTANT_FUTURE = Time(INT64_MAX);
const Time DISTANT_PAST = Time(INT64_MIN);

/** A time represented as a count from the host.
 */
struct Ticks {
    uint64_t intrinsic;

    Ticks(void) = default;

    Ticks(uint64_t x) : intrinsic(x) {}

    inline Time operator()(TimeCalibration &calibration) const
    {
        __int128_t tmp = intrinsic;

        auto gainBias = calibration.getGainBias();

        tmp *= gainBias.first;
        tmp >>= 64;
        tmp += gainBias.second;
        return Time{static_cast<int64_t>(tmp)};
    }
};

static inline Ticks getTicks(void) {
    return Ticks{__builtin_ia32_rdtsc()};
}


};};
