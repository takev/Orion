
#include <algorithm>
#include <errno.h>
#include <sys/epoll.h>

#include "RunLoop.hpp"
#include "Application.hpp"

namespace Orion {
namespace Rigel {

const int MAX_EPOLL_EVENTS = 10;
const int EPOLL_DEFAULT_TIMEOUT = 10;

RunLoop::RunLoop(void) :
    epollFD(-1), pollEventHandlers(), timerEventHandlers()
{
    if ((epollFD = epoll_create1(0)) == -1) {
        BOOST_THROW_EXCEPTION(runloop_epoll_error() << boost::errinfo_errno(errno));
    }

}

RunLoop::RunLoop(const RunLoop &other) :
    epollFD(-1), pollEventHandlers(), timerEventHandlers()
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
    return !pollEventHandlers.empty();
}

void RunLoop::add(const std::shared_ptr<EventHandler> &eventHandler)
{
    if (pollEventHandlers.count(eventHandler->fileDescriptor()) == 1) {
        BOOST_THROW_EXCEPTION(runloop_event_handler_error());
    }

    eventHandler->parent = shared_from_this();
    pollEventHandlers[eventHandler->fileDescriptor()] = eventHandler;
    updatePoll(eventHandler, EPOLL_CTL_ADD);
}

void RunLoop::remove(const std::shared_ptr<EventHandler> &eventHandler)
{
    if (pollEventHandlers.count(eventHandler->fileDescriptor()) == 0) {
        BOOST_THROW_EXCEPTION(runloop_event_handler_error());
    }

    updatePoll(eventHandler, EPOLL_CTL_DEL);
    pollEventHandlers.erase(eventHandler->fileDescriptor());
    eventHandler->parent.reset();
}

void RunLoop::updatePoll(const std::shared_ptr<EventHandler> &eventHandler, int op)
{
    epoll_event event;

    event.data.fd = eventHandler->fileDescriptor();
    event.events = EPOLLONESHOT;
    event.events|= eventHandler->wantToRead() ? EPOLLIN : 0;
    event.events|= eventHandler->wantToWrite() ? EPOLLOUT : 0;

    if ((epoll_ctl(epollFD, op, event.data.fd, &event)) == -1) {
        BOOST_THROW_EXCEPTION(runloop_epoll_error() << boost::errinfo_errno(errno));
    }

    auto triggerTime = eventHandler->wantToWake();

    if (triggerTime == DISTANT_FUTURE) {
        return;
    }

    while (true) {
        auto i = timerEventHandlers.find(triggerTime);

        if (i == timerEventHandlers.end()) {
            // Empty entry found, insert the eventHandler in the timer list.
            timerEventHandlers[triggerTime] = eventHandler;
            return;

        } else if (auto otherEventHandlers = i->second.lock()) { 
            if (otherEventHandlers == eventHandler) {
                // Found the current eventHandler in the timer list.
                return;
            }
        }

        triggerTime++;
    }
}

int RunLoop::runTimers(void)
{
    while (!timerEventHandlers.empty()) {
        auto currentTime = app->getTime();

        auto i = timerEventHandlers.begin();
        auto triggerTime = i->first;
        auto eventHandler = i->second;

        if (currentTime < triggerTime) {
            // All triggers are ordered, so we can break early.
            return (triggerTime - currentTime).toMilliseconds();
        }

        if (auto _eventHandler = eventHandler.lock()) {
            _eventHandler->handleWake(triggerTime, currentTime);
        } else {
            BOOST_THROW_EXCEPTION(runloop_event_handler_error());
        }
        timerEventHandlers.erase(i);
    }

    return EPOLL_DEFAULT_TIMEOUT;
}

void RunLoop::run(void)
{
    epoll_event events[MAX_EPOLL_EVENTS];
    int nr_events;

    // Find the next nearest timer, and run expired timerEventHandlers.
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

    // Best resolution for timerEventHandlers is right after the epoll() exists.
    runTimers();

    // Handle events.
    nr_events = std::min(nr_events, MAX_EPOLL_EVENTS);
    for (auto i = 0; i < nr_events; i++) {
        auto eventHandler = pollEventHandlers[events[i].data.fd];

        if (events[i].events & EPOLLERR) {
            eventHandler->handleError();
        }
        if (events[i].events & EPOLLOUT) {
            eventHandler->handleWrite();
        }
        if (events[i].events & EPOLLIN || events[i].events & EPOLLHUP) {
            eventHandler->handleRead();
        }

        updatePoll(eventHandler, EPOLL_CTL_MOD);
    }
}

void RunLoop::loop(void)
{
    while (isRunning()) {
        run();
    }
}

};};
