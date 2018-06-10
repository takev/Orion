
#include "UnixDatagramListener.hpp"

namespace Orion {
namespace Rigel {

UnixDatagramListener::UnixDatagramListener(const boost::path &path)
{

}

virtual UnixDatagramListener::~UnixDatagramListener()
{

}

/** Return file descriptor to poll on.
 *
 * @return valid file descriptor to poll on.
 */
virtual int UnixDatagramListener::fileDescriptor(void) const
{
}

/** Event handler wants to read from file descriptor.
 */
virtual bool UnixDatagramListener::wantToRead(void) const;
{
    return true;
}

/** Event handler wants to write to file descriptor.
 */
virtual bool UnixDatagramListener::wantToWrite(void) const;
{
    return false;
}

/** Event handler wants to wake at a specific time.
 */
virtual Time UnixDatagramListener::wantToWake(void) const;
{
    return FAR_FUTURE;
}

/** Data is ready to be read from the file descriptor.
 * Possible the the peer has closed the writing side of their
 * file descriptor.
 */
virtual void UnixDatagramListener::handleRead(void);
{
}

/** Data is ready to be written to the file descriptor.
 */
virtual void UnixDatagramListener::handleWrite(void);
{
}

/** An error occured on the file descriptor.
 */
virtual void UnixDatagramListener::handleError(void);
{
}

/** Timer has triggered.
 */
virtual void UnixDatagramListener::handleWake(Time triggerTime, Time currentTime);
{
}

};};
