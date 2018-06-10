
#include <algorithm>

#include "RunLoop.hpp"
#include "Application.hpp"

namespace Orion {
namespace Rigel {

EventHandler::EventHandler(void) :
    parent()
{
}

EventHandler::~EventHandler()
{
}

void EventHandler::updatePoll(void)
{
    if (auto tmp = parent.lock()) {
        tmp->updatePoll(shared_from_this());
    } else {
        BOOST_THROW_EXCEPTION(runloop_event_handler_error());
    }
}

};};
