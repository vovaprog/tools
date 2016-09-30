#include <signal.h>

#include <Server.h>
#include <Log.h>

Server srv;
std::atomic_bool runFlag;

static void sig_int_handler(int i)
{
    printf("sig int\n");

    runFlag.store(false);
}

int main(int argc, char** argv)
{
    runFlag.store(true);
    signal(SIGINT, sig_int_handler);

    ServerParameters params;
    params.maxClients = 10;
    //strcpy(params.rootFolder, "/media/vova/programs/programs/git/tools/src/http_server/build/data");
    strcpy(params.rootFolder, "/home/vlads/programs/tools/src/http_server/build/data");
    params.logLevel = Log::Level::debug;

    params.uwsgiApplications.push_back("/gallery");
    params.uwsgiApplications.push_back("/calendar");

    params.threadCount = 3;
    params.httpPorts.push_back(7000);
    params.httpPorts.push_back(11000);

	params.httpsPorts.push_back(1443);

    srv.start(params);

    while(runFlag.load())
    {
        srv.logStats();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    srv.stop();

    return 0;
}
