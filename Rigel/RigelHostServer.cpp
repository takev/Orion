
#include "Application.hpp"

using namespace std;
using namespace Orion::Rigel;

int main(int argc, const char *argv[])
{
    app = make_shared<Application>(argc, argv);
    app->connectToNameServer();
    app->createSharedMemory();
    app->createHostServerListener();

    while (app->runloop->isRunning()) {
        app->runloop->run();
    }
}

//int main(int argc, const char *argv[])
//{
//    app = make_shared<Application>(argc, argv);
//    app.connectToHostServer();
//    app.getSharedMemoryFromHostServer();
//    app.loop();
//}

