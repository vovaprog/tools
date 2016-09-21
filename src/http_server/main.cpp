#include <sys/types.h>
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
#include <TransferRingBuffer.h>


int epollFd = -1;





struct DescriptorToExecutor {
    int fd;
    Executor *pExecutor;
};



int init()
{
    serverSockFd = socketListen(7000);
    if(serverSockFd < 0)
    {
        return -1;
    }
    
    
    epollFd = epoll_create1(0);
    if(epollFd == -1)
    {
        perror("epoll_create1 failed");
        destroy();
        return -1;
    }


    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serverSockFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSockFd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        destroy();
        return -1;
    }

    epoll_event events[maxEvents];

    while(true)
    {
        int nEvents = epoll_wait(epollFd, events, maxEvents, -1);
        if(nEvents == -1)
        {
            perror("epoll_wait failed");
            destroy();
            return -1;
        }

        for(int i = 0; i < nEvents; ++i)
        {
            DescriptorToExecutor *d2e = events[i].data.ptr;            
            
            Executor *pExecutor = d2e->pExecutor;
            
            ProcessResult result = pExecutor->process(d2e->fd, events[i].events);
            
            handleResult(result);
        }
    }
}

int createExecutor(ProcessResult)
{


}

int handleResult(ProcessResult result)
{
	if(result.action == createExecutor)
	{
		return createExecutor(result);
	}
	printf("unknown action\n");
	return 0;
}


int main(int argc, char** argv)
{
    signal(SIGINT, sig_int_handler);

    server();

    return 0;
}
