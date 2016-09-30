#include <signal.h>

#include <Server.h>
#include <Log.h>

Server srv;

static void sig_int_handler(int i)
{
	printf("sig int\n");

    srv.destroy();

    exit(-1);
}

int main(int argc, char** argv)
{
    signal(SIGINT, sig_int_handler);

    ServerParameters params;
    params.port = 7000;
    params.maxClients = 10;
	//strcpy(params.rootFolder, "/media/vova/programs/programs/git/tools/src/http_server/build/data");
	strcpy(params.rootFolder, "/home/vlads/programs/tools/src/http_server/build/data");
    params.logLevel = Log::Level::debug;

    strcpy(params.wsgiApplications[0], "/gallery");
    strcpy(params.wsgiApplications[1], "/calendar");
    params.wsgiApplications[2][0] = 0;

	params.threadCount = 3;
	params.httpPorts.push_back(7000);

    srv.run(params);

    return 0;
}
