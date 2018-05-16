#pragma once

#include <cstdint>
#include <atomic>
#include <boost/filesystem.hpp>
#include <x86intrin.h>

#include "Time.hpp"
#include "Identifiers.hpp"
//#include "Logging.hpp"

namespace Orion {
namespace Rigel {

const_expr size_t cachePaddingWidth(size_t previousObjectWidth)
{
    size_t tmp = previousObjectWidth % CACHE_LINE_WIDTH;
    return CACHE_LINE_WIDTH - tmp;
}

enum class HostServicesSharedMemoryState {
}

/**
 * For performace reasons every field should be aligned to a cache line.
 * This is because a write in a field will cause a cache invalidation on all other CPUs/Cores.
 */
struct HostServicesSharedMemory {
    HostServicesSharedMemoryState   state;
    char                            padding1[cachePaddingWidth(sizeof (HostServicesSharedMemoryState))];

    Identifiers                     identifiers;
    char                            padding2[cachePaddingWidth(sizeof (Identifier))];

    TimeCalibration                 timeCalibration;
    char                            padding3[cachePaddingWidth(sizeof (TimeCalibration))];

    //Logging                         logging;
    //char                            padding4[cachePaddingWidth(sizeof (Logging))];
}

/** Application singleton.
 * An instance of this class can be used to call different application level services.
 * 
 */
class Application final {
private:
    boost::filesystem::path shared_mem_path;

    HostServicesSharedMemory *shared_mem;


public:
    Application(const char *argv[], int argc);
    ~Application();

    /** Get the cluster host ID.
     *
     * @return A 12 bit cluster host ID.
     */
    inline Host getHostId()
    {
        return shared_mem->identifiers.getHostID();
    }

    /** Get the time.
     *
     * @return Number of nanoseconds since 2000.
     */
    inline Time getTime()
    {
        auto ticks = Time::getTicks()

        return ticks(shared_mem->timeCalibration);
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
        return identifiers.getUniqueID(getTime());
    }

    /**
     * @see Logging::Logging::log()
     */
    //template<typename FIRST, typename... REST>
    //inline void log(std::string file, uint64_t line, std::string msg, REST... rest)
    //{
    //    shared_mem->logging.log(file, line, msg, rest);
    //}
    
}

};};

/** HostServices is a singleton.
 * It needs to be initialized as soon as the shared_mem_path is known.
 */
std::shared_ptr<Rigel::Application::Application> app;
