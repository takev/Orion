#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <boost/exception/all.hpp>
#include <sys/epoll.h>

#include "Time.hpp"
#include "EventHandler.hpp"

namespace Orion {
namespace Rigel {

struct runloop_error: virtual boost::exception {};
struct runloop_epoll_error: virtual runloop_error, virtual std::exception {};
struct runloop_event_handler_error: virtual runloop_error, virtual std::exception {};

class RunLoop : public std::enable_shared_from_this<RunLoop> {
private:
    int epollFD;
    std::unordered_map<int, std::shared_ptr<EventHandler>> pollEventHandlers;
    std::map<Time, std::weak_ptr<EventHandler>> timerEventHandlers;

public:
    RunLoop(void);
    RunLoop(const RunLoop &other);
    ~RunLoop();

    /** Update runloop to check for what to poll.
     */
    void updatePoll(const std::shared_ptr<EventHandler> &eventHandler, int op=EPOLL_CTL_MOD);

    /** Add an event handler to the run loop.
     */
    void add(const std::shared_ptr<EventHandler> &eventHandler);

    /** Remove an event handler to the run loop.
     */
    void remove(const std::shared_ptr<EventHandler> &eventHandler);

    /** There are event handlers in the run loop.
     */
    bool isRunning(void);

    /** Run all event handler whos trigger time has expired.
     *
     * @return Number of milliseconds to wait until next timer event.
     */
    int runTimers(void);

    /** Run a single iteration.
     */
    void run(void);


    /** Continue running until there is nothing to run.
     */
    void loop(void);
};

};};
