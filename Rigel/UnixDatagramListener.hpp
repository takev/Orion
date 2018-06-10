#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <boost/exception/all.hpp>

#include "EventHandler.hpp"

namespace Orion {
namespace Rigel {

class UnixDatagramListener : public EventHandler {
public:
    int fd;

    UnixDatagramListener(const boost::path &path);
    virtual ~UnixDatagramListener();

    /** Return file descriptor to poll on.
     *
     * @return valid file descriptor to poll on.
     */
    virtual int fileDescriptor(void) const;

    /** Event handler wants to read from file descriptor.
     */
    virtual bool wantToRead(void) const;

    /** Event handler wants to write to file descriptor.
     */
    virtual bool wantToWrite(void) const;

    /** Event handler wants to wake at a specific time.
     */
    virtual Time wantToWake(void) const;

    /** Data is ready to be read from the file descriptor.
     * Possible the the peer has closed the writing side of their
     * file descriptor.
     */
    virtual void handleRead(void);

    /** Data is ready to be written to the file descriptor.
     */
    virtual void handleWrite(void);

    /** An error occured on the file descriptor.
     */
    virtual void handleError(void);

    /** Timer has triggered.
     */
    virtual void handleWake(Time triggerTime, Time currentTime);
};

};};
