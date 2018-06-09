#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <boost/exception/all.hpp>

#include "Time.hpp"

namespace Orion {
namespace Rigel {

struct runloop_error: virtual boost::exception {};
struct runloop_epoll_error: virtual runloop_error, virtual std::exception {};
struct runloop_object_error: virtual runloop_error, virtual std::exception {};

class RunLoop;

// enabled_shared_from_this has a non-virtual destructor for performance reasons.
// supress effc++ false positive.
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class RunLoopObject : public std::enable_shared_from_this<RunLoopObject> {
#pragma GCC diagnostic warning "-Wnon-virtual-dtor"
private:
    std::weak_ptr<RunLoop> parent;

public:
    RunLoopObject(void);
    virtual ~RunLoopObject();

    virtual void setParent(const std::shared_ptr<RunLoop> &runLoop);

    virtual void updatePoll(void);

    virtual int fileDescriptor(void) = 0;

    virtual bool wantToRead(void) = 0;
    virtual bool wantToWrite(void) = 0;
    virtual Time wantToWake(void) = 0;

    virtual void handleRead(void) = 0;
    virtual void handleWrite(void) = 0;
    virtual void handleError(void) = 0;
    virtual void handleWake(Time triggerTime, Time currentTime) = 0;
};

class RunLoop : public std::enable_shared_from_this<RunLoop> {
private:
    int epollFD;
    std::unordered_map<int, std::shared_ptr<RunLoopObject>> pollObjects;
    std::map<Time, std::weak_ptr<RunLoopObject>> timerObjects;

public:
    RunLoop(void);
    RunLoop(const RunLoop &other);
    ~RunLoop();

    void updatePoll(const std::shared_ptr<RunLoopObject> &object, int op);

    void add(const std::shared_ptr<RunLoopObject> &object);
    void remove(const std::shared_ptr<RunLoopObject> &object);

    bool isRunning(void);
    int runTimers(void);
    void run(void);
    void loop(void);

};

};};
