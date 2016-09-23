/*#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <NetworkUtils.h>
#include <TransferRingBuffer.h>*/

#include <Server.h>

int main(int argc, char** argv)
{	
    signal(SIGINT, sig_int_handler);

	Server srv;
	Server::Parameters params;
	params.port = 7000;
	params.maxClients = 10;

	srv.run(params);

    return 0;
}
