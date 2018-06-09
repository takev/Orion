
#include "Application.hpp"

namespace Orion {
namespace Rigel {

Application::Application(int argc, const char *argv[]) :
    sharedMemory(NULL), runloop()
{
}

Application::~Application()
{
}

void Application::connectToNameServer(void)
{
}

void Application::connectToHostServer(void)
{
}

void Application::createSharedMemory(void)
{
}

void Application::createHostServerListener(void)
{
}

std::shared_ptr<Application> app;

};};
