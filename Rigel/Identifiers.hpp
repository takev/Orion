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
#include <boost/exception/all.hpp>
#include "Time.hpp"
#include "int_utils.hpp"

namespace Orion {
namespace Rigel {

struct host_id_overflow: virtual boost::exception, virtual std::exception {};

struct HostID {
    uint16_t intrinsic;

    HostID(void) = default;

    HostID(uint16_t x) : intrinsic(x) {}

    inline void check(void)
    {
        if (intrinsic > 0xfff) {
            BOOST_THROW_EXCEPTION(host_id_overflow());
        }
    }
};

/** A cluster wide unique ID.
 * The unique ID is build up as follows:
 *  * 12 bits cluster-host id
 *  * 52 bits time (number of (4096*nanoseconds) since 2000).
 */
struct UniqueID {
    uint64_t intrinsic;

    UniqueID(void) = default;

    UniqueID(uint64_t x) : intrinsic(x) {}

    inline HostID getHostID(void)
    {
        return HostID(static_cast<uint16_t>((intrinsic & 0xfff0'0000'0000'0000ULL) >> 52));
    }

    inline void setHostID(HostID hostID)
    {
        hostID.check();

        intrinsic &= 0x000f'ffff'ffff'ffffULL;
        intrinsic |= static_cast<uint64_t>(hostID.intrinsic) << 52;
    }

    inline Time getTime(void)
    {
        return Time((intrinsic & 0x000f'ffff'ffff'ffff) << 12);
    }

    inline void setTime(Time time)
    {
        intrinsic &= 0xfff0'0000'0000'0000ULL;
        intrinsic |= time.intrinsic >> 12;
    }

    inline UniqueID operator()(HostID hostID)
    {
        auto tmp = UniqueID(intrinsic);
        tmp.setHostID(hostID);
        return tmp;
    }

    inline UniqueID operator()(Time time)
    {
        auto tmp = UniqueID(intrinsic);
        tmp.setTime(time);
        return tmp;
    }

    inline bool operator>(UniqueID other)
    {
        return intrinsic > other.intrinsic;
    }
};

struct Identifiers {
    std::atomic<uint64_t> lastUniqueID;  ///< The intrinsic value of a UniqueID.

    /** Set the host ID of this host.
     *
     * @param hostID Host ID of this host.
     */
    inline void setHostID(HostID hostID)
    {
        hostID.check();

        uint64_t id;
        uint64_t newId;
        do {
            id = lastUniqueID.load();
            auto tmp = UniqueID(id);
            newId = tmp(hostID).intrinsic;
        } while (lastUniqueID.compare_exchange_weak(id, newId));
    }

    /** Get the hostID.
     * The host ID is a 12 bit number representing a unique ID for each host
     * in a cluster.
     *
     * @return 12 bit unique id for this host on the cluster.
     */
    inline HostID getHostID(void)
    {
        UniqueID id = UniqueID(lastUniqueID.load());
        return id.getHostID();
    }

    /** Get a cluster wide unique ID.
     * The current time is only updated opertunistically when the time has advanced
     * enough and when there is no contention, ensuring wait-free operation.
     *
     * @param time The current time.
     * @return A cluster wide unique ID.
     */
    inline UniqueID getUniqueID(Time time)
    {
        // At this point we already have a unique ID.
        auto id = UniqueID{++lastUniqueID};

        // Create a unique id based on the time stamp given.
        auto new_id = id(time);

        // Opertunistically try to use the new_id.
        // If it fails we can always use the unique ID we got earlier in the function.
        bool valid_new_id = new_id > id && lastUniqueID.compare_exchange_weak(id.intrinsic, new_id.intrinsic);

        return valid_new_id ? new_id : id;
    }
} __attribute__ ((aligned(CACHE_LINE_SIZE)));

};};
