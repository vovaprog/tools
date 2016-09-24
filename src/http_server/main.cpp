#include <signal.h>

#include <Server.h>

Server srv;

static void sig_int_handler(int i)
{
	printf("sig int handler\n");

    srv.destroy();

	exit(-1);
}

int main(int argc, char** argv)
{	
    signal(SIGINT, sig_int_handler);

    ServerParameters params;
	params.port = 7000;
	params.maxClients = 10;
    strcpy(params.rootFolder, "/media/vova/programs/programs/git/tools/src/http_server/build/data");

	srv.run(params);

    return 0;
}
