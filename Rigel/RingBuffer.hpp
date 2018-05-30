#pragma once

namespace Orion {
namespace Rigel {

enum class RingBufferItemState {
    EMPTY,
    WRITING,
    READY,
    READING
};

tremplatr<typename T>
struct RingBufferItem {
    RingBufferItemState  state;
    T                    data;
};

#ifndef CACHE_LINE_WIDTH
#define CACHE_LINE_WIDTH 128
#endif

struct DoubleCounter {
    atomic<uint32_t>   intrinsic;

    inline std::pair<uint16_t, uint16_t> load(void) {
        uint32_t v = intrinsic.load();

        return std::pair<uint16_t, uint16_t>{static_cast<uint16_t>(v), static_cast<uint16_t(v >> 16)};
    }

    inline void store(uint16_t first, uint16_t second) {
        uint32_t v = static_cast<uint32_t>(second) << 16 | static_cast<uint32_t>(first);

        intrinsic.store(v);
    }

    inline bool compare_exchange_weak(uint16_t oldFirst, uint16_t oldSecond, uint16_t newFirst, uint16_t newSecond) {
        uint32_t oldValue = static_cast<uint32_t>(oldSecond) << 16 | static_cast<uint32_t>(oldFirst);
        uint32_t newValue = static_cast<uint32_t>(newSecond) << 16 | static_cast<uint32_t>(newFirst);

        return intrinsic.compare_exchange_weak(oldValue, newValue);
    }

    inline void wait(uint16_t oldFirst, uint16_t oldSecond, Duration timeout) {
        uint32_t oldValue = static_cast<uint32_t>(oldSecond) << 16 | static_cast<uint32_t>(oldFirst);

        futex(reinterpret_cast<int *>(&intrinsic), FUTEX_WAIT, static_cast<int>(oldValue), NULL, NULL, 0);
    }

    inline void notify_all(void) {
        futex(reinterpret_cast<int *>(&intrinsic), FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
    }
}

template<typename T, uint16_t nrElements>
struct RingBuffer {
    DoubleCounter       readWriteCounters;
    char                padding1[CACHE_LINE_WIDTH - sizeof (uint32_t)];

    RingBufferItem<T>   items[nrElements];


    RingBufferItem &getWriteCursor(void)
    {
        uint32_t currentReadWriteCounter;

        currentReadWriteCounter = readWriteCounter;

    }

    RingBufferItem &getReadCursor(void)
    {
        auto currentCounters = readWriteCounters.load();
        int32_t distance = static_cast<int32_t>(writeCounter - readCounter);
    }
};

};};
