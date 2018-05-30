#pragma once
#include <cstdint>
#include <atomic>
#include <boost/filesystem.hpp>
#include <x86intrin.h>

namespace Orion {
namespace Rigel {
namespace Time {

struct TimeCalibration {
    __uint128_t intrinsic;

    inline setGainBias(uint64_t gain, uint64_t bias)
    {
        uint128_t newTimeAdjust = (static_cast<__uint128_t>(gain) << 64) | static_cast<__uint128_t>(bias);

        // The 128-bit CAS instruction is the only way to write 128 bits atomically.
        do {
            uint128_t oldTimeAdjust = timeAdjust.load();
        } while (!__sync_bool_compare_and_swap(&timeAdjust, oldTimeAdjust, newTimeAdjust));
    }

    /** Convert a host tick to time.
     * This function is useful if you need to get a low latency timestamp,
     * and can convert to actual time on a slower path.
     */
    inline std::pair<uint64_t, uint64_t> getGainBias(void)
    {
        // The 128-bit CAS instruction is the only way to read 128 bits atomically.
        // An actual swap is never executed, due to the 0 compare value.
        // This is still wait-free, because we do not need to loop.
        uint128_t currentTimeAdjust = __sync_val_compare_and_swap(&timeAdjust, 0, 0);

        uint64_t gain = static_cast<uint64_t>(currentTimeAdjust >> 64);
        uint64_t bias = static_cast<uint64_t>(currentTimeAdjust);

        return std::pair<uint64_t, uint64_t>{gain, bias};
    }
}

/** A duration in nanoseconds.
 * The duration is incremented based on atomic clocks, without
 * leap seconds or drift.
 */
struct Duration {
    int64_t intrinsic;
}

/** The time in TAI nanoseconds since January 1st of the year 2000.
 * TAI is used so we do not have to acount for clock drift during
 * days with leap seconds.
 */
struct Time {
    uint64_t intrinsic;

    inline Duration operator-(const Time &other) {
        return Duration{static_cast<int64_t>(intrinsic - other.intrinsic)}
    }

    inline Time operator+(const Duration &other) {
        return Time(intrinsic + other.intrinsic);
    }
}

/** A time represented as a count from the host.
 */
struct Ticks {
    uint64_t intrinsic;

    inline Time operator()(TimeCalibration &calibration)
    {
        __uint128_t tmp = intrinsic;

        auto gainBias = calibration.getGainBias();

        tmp *= gainBias.first;
        tmp >>= 64;
        tmp += gainBias.second;
        return Time{static_cast<uint64_t>(tmp)};
    }
}

Ticks getTicks(void) {
    return Ticks{__builtin_ia32_rdtsc()};
}




};};};
