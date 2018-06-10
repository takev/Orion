#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <boost/exception/all.hpp>

#include "Time.hpp"

namespace Orion {
namespace Rigel {

class RunLoop;

// enabled_shared_from_this has a non-virtual destructor for performance reasons.
// supress effc++ false positive.
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class EventHandler : public std::enable_shared_from_this<EventHandler> {
#pragma GCC diagnostic warning "-Wnon-virtual-dtor"
public:
    std::weak_ptr<RunLoop> parent;

    EventHandler(void);
    virtual ~EventHandler();

    /** Update the parent runLoop to re-evaluate polling.
     */
    virtual void updatePoll(void);

    /** Return file descriptor to poll on.
     *
     * @return valid file descriptor to poll on.
     */
    virtual int fileDescriptor(void) const = 0;

    /** Event handler wants to read from file descriptor.
     */
    virtual bool wantToRead(void) const = 0;

    /** Event handler wants to write to file descriptor.
     */
    virtual bool wantToWrite(void) const = 0;

    /** Event handler wants to wake at a specific time.
     */
    virtual Time wantToWake(void) const = 0;

    /** Data is ready to be read from the file descriptor.
     * Possible the the peer has closed the writing side of their
     * file descriptor.
     */
    virtual void handleRead(void) = 0;

    /** Data is ready to be written to the file descriptor.
     */
    virtual void handleWrite(void) = 0;

    /** An error occured on the file descriptor.
     */
    virtual void handleError(void) = 0;

    /** Timer has triggered.
     */
    virtual void handleWake(Time triggerTime, Time currentTime) = 0;
};

};};
