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
#include <memory>
#include <boost/filesystem.hpp>
#include <x86intrin.h>

#include "Time.hpp"
#include "Identifiers.hpp"
#include "RunLoop.hpp"
//#include "Logging.hpp"

namespace Orion {
namespace Rigel {

/**
 * For performace reasons every field should be aligned to a cache line.
 * This is because a write in a field will cause a cache invalidation on all other CPUs/Cores.
 */
struct HostServicesSharedMemory {
    Identifiers                     identifiers;
    TimeCalibration                 timeCalibration;

    //Logging                         logging;
};

/** Application singleton.
 * An instance of this class can be used to call different application level services.
 * 
 */
class Application final {
public:
    HostServicesSharedMemory *sharedMemory;
    std::shared_ptr<RunLoop> runloop;

    Application(int argc, const char *argv[]);

    Application(const Application &other) : sharedMemory(other.sharedMemory), runloop() {}

    ~Application();

    Application &operator=(const Application &other)
    {
        sharedMemory = other.sharedMemory;
        return *this;
    }

    /** Get the cluster host ID.
     *
     * @return A 12 bit cluster host ID.
     */
    inline HostID getHostId()
    {
        return sharedMemory->identifiers.getHostID();
    }

    /** Get the time.
     *
     * @return Number of nanoseconds since 2000.
     */
    inline Time getTime()
    {
        auto ticks = getTicks();

        return ticks(sharedMemory->timeCalibration);
    }

    /** A cluster wide unique ID.
     * The unique is build up as follows:
     *  * 12 bits cluster-host id
     *  * 52 bits time (microseconds since 2000).
     *
     * The time will be incremented if multiple getUniqueID() requests are done
     * in the same microsecond.
     *
     * This function is wait-free; i.e. every thread will make progress (does not loop).
     *
     * @return A cluster wide unique ID.
     */
    inline UniqueID getUniqueID()
    {
        return sharedMemory->identifiers.getUniqueID(getTime());
    }

    /**
     * @see Logging::Logging::log()
     */
    //template<typename FIRST, typename... REST>
    //inline void log(std::string file, uint64_t line, std::string msg, REST... rest)
    //{
    //    sharedMemory->logging.log(file, line, msg, rest);
    //}

    void connectToNameServer(void);

    void connectToHostServer(void);

    void createSharedMemory(void);

    void createHostServerListener(void);
    
};

/** HostServices is a singleton.
 * It needs to be initialized at the start of the application.
 */
extern std::shared_ptr<Application> app;

};};

