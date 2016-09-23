#include <Server.h>

/*static void sig_int_handler(int i)
{
	printf("sig int handler\n");

	if(serverSockFd > 0)
	{
		close(serverSockFd);
	}

	exit(-1);
}
*/

int main(int argc, char** argv)
{	
	//signal(SIGINT, sig_int_handler);

	Server srv;
	Server::Parameters params;
	params.port = 7000;
	params.maxClients = 10;

	srv.run(params);

    return 0;
}
