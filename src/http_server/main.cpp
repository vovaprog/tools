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
	params.maxClients = 1000;

	//params.rootFolder = "/media/vova/programs/programs/git/tools/src/http_server/build/data";
	params.rootFolder = "/home/vlads/programs/tools/src/http_server/build/data";

	//params.logLevel = Log::Level::debug;
	params.logLevel = Log::Level::info;
	params.logType = Log::Type::mmap;

    params.uwsgiApplications.push_back("/gallery");
    params.uwsgiApplications.push_back("/calendar");

	params.threadCount = 3;
    params.httpPorts.push_back(7000);
	//params.httpPorts.push_back(80);

	params.httpsPorts.push_back(1443);
	//params.httpsPorts.push_back(443);

	params.executorTimeoutMilliseconds = 10000;

    if(srv.start(params) != 0)
    {
        return -1;
    }

    while(runFlag.load())
    {
        srv.logStats();
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    srv.stop();

    return 0;
}
