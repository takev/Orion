
#include <algorithm>
#include <errno.h>
#include <sys/epoll.h>

#include "RunLoop.hpp"
#include "Application.hpp"

namespace Orion {
namespace Rigel {

const int MAX_EPOLL_EVENTS = 10;
const int EPOLL_DEFAULT_TIMEOUT = 10;

RunLoopObject::RunLoopObject(void) :
    parent()
{
}

RunLoopObject::~RunLoopObject()
{
}

void RunLoopObject::setParent(const std::shared_ptr<RunLoop> &runLoop)
{
    parent = runLoop;
}

void RunLoopObject::updatePoll(void)
{
    if (auto tmp = parent.lock()) {
        tmp->updatePoll(shared_from_this(), EPOLL_CTL_MOD);
    } else {
        BOOST_THROW_EXCEPTION(runloop_object_error());
    }
}

RunLoop::RunLoop(void) :
    epollFD(-1), pollObjects(), timerObjects()
{
    if ((epollFD = epoll_create1(0)) == -1) {
        BOOST_THROW_EXCEPTION(runloop_epoll_error() << boost::errinfo_errno(errno));
    }

}

RunLoop::RunLoop(const RunLoop &other) :
    epollFD(-1), pollObjects(), timerObjects()
{
    if (other.epollFD != -1) {
        if ((epollFD = dup(other.epollFD)) == -1) {
            BOOST_THROW_EXCEPTION(runloop_epoll_error() << boost::errinfo_errno(errno));
        }
    }
}

RunLoop::~RunLoop()
{
    if (epollFD != -1) {
        if (close(epollFD) == -1) {
            BOOST_THROW_EXCEPTION(runloop_epoll_error() << boost::errinfo_errno(errno));
        }
        epollFD = -1;
    }
}

bool RunLoop::isRunning(void)
{
    return !pollObjects.empty();
}

void RunLoop::add(const std::shared_ptr<RunLoopObject> &object)
{
    if (pollObjects.count(object->fileDescriptor()) == 1) {
        BOOST_THROW_EXCEPTION(runloop_object_error());
    }

    object->setParent(shared_from_this());
    pollObjects[object->fileDescriptor()] = object;

    updatePoll(object, EPOLL_CTL_ADD);
}

void RunLoop::remove(const std::shared_ptr<RunLoopObject> &object)
{
    if (pollObjects.count(object->fileDescriptor()) == 0) {
        BOOST_THROW_EXCEPTION(runloop_object_error());
    }

    updatePoll(object, EPOLL_CTL_DEL);
    pollObjects.erase(object->fileDescriptor());
}

void RunLoop::updatePoll(const std::shared_ptr<RunLoopObject> &object, int op)
{
    epoll_event event;

    event.data.fd = object->fileDescriptor();
    event.events = EPOLLONESHOT;
    event.events|= object->wantToRead() ? EPOLLIN : 0;
    event.events|= object->wantToWrite() ? EPOLLOUT : 0;

    if ((epoll_ctl(epollFD, op, event.data.fd, &event)) == -1) {
        BOOST_THROW_EXCEPTION(runloop_epoll_error() << boost::errinfo_errno(errno));
    }

    auto triggerTime = object->wantToWake();

    if (triggerTime == DISTANT_FUTURE) {
        return;
    }

    while (true) {
        auto i = timerObjects.find(triggerTime);

        if (i == timerObjects.end()) {
            // Empty entry found, insert the object in the timer list.
            timerObjects[triggerTime] = object;
            return;

        } else if (auto otherObject = i->second.lock()) { 
            if (otherObject == object) {
                // Found the current object in the timer list.
                return;
            }
        }

        triggerTime++;
    }
}

int RunLoop::runTimers(void)
{
    while (!timerObjects.empty()) {
        auto currentTime = app->getTime();

        auto i = timerObjects.begin();
        auto triggerTime = i->first;
        auto object = i->second;

        if (currentTime < triggerTime) {
            // All triggers are ordered, so we can break early.
            return (triggerTime - currentTime).toMilliseconds();
        }

        if (auto _object = object.lock()) {
            _object->handleWake(triggerTime, currentTime);
        } else {
            BOOST_THROW_EXCEPTION(runloop_object_error());
        }
        timerObjects.erase(i);
    }

    return EPOLL_DEFAULT_TIMEOUT;
}

void RunLoop::run(void)
{
    epoll_event events[MAX_EPOLL_EVENTS];
    int nr_events;

    // Find the next nearest timer, and run expired timerObjects.
    auto timeout = runTimers();

    if ((nr_events = epoll_wait(epollFD, events, MAX_EPOLL_EVENTS, timeout)) == -1) {
        switch (errno) {
        case EINTR:
            nr_events = 0;
            break;
        default:
            BOOST_THROW_EXCEPTION(runloop_epoll_error() << boost::errinfo_errno(errno));
        }
    }

    // Best resolution for timerObjects is right after the epoll() exists.
    runTimers();

    // Handle events.
    nr_events = std::min(nr_events, MAX_EPOLL_EVENTS);
    for (auto i = 0; i < nr_events; i++) {
        auto object = pollObjects[events[i].data.fd];

        if (events[i].events & EPOLLERR) {
            object->handleError();
        }
        if (events[i].events & EPOLLOUT) {
            object->handleWrite();
        }
        if (events[i].events & EPOLLIN || events[i].events & EPOLLHUP) {
            object->handleRead();
        }

        updatePoll(object, EPOLL_CTL_MOD);
    }
}

void RunLoop::loop(void)
{
    while (isRunning()) {
        run();
    }
}

};};
