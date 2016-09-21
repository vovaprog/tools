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

static int serverSockFd = -1;
static bool withCheck = false;
static const int BUF_SIZE = 1000000;
static const int maxClients = 20;

struct ClientData
{
    int fd = -1;
    TransferRingBuffer buffer;    
};

ClientData clientDatas[maxClients];


static void sig_int_handler(int i)
{
    printf("sig int handler\n");

    if(serverSockFd > 0)
    {
        close(serverSockFd);
    }

    exit(-1);
}


static void checkBuffer(void *buffer, int size)
{
    unsigned char *buf = static_cast<unsigned char*>(buffer);

    for(int i = 0; i < size - 1; ++i)
    {
        if(!((buf[i] == 255 && buf[i + 1] == 0) || buf[i] + 1 == buf[i + 1]))
        {
            printf("invalid data:   buf[%d]=%d   buf[%d]=%d\n", i, (int)buf[i], i + 1, (int)buf[i + 1]);
            for(int j = i - 10; j < i + 10; ++j)
            {
                if(j >= 0 && j < size)
                {
                    printf("buf[%d] = %d\n", j, buf[j]);
                }
            }

            exit(-1);
        }
    }
}


static bool processAccept(int epollFd)
{
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    int clientSock = accept(serverSockFd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    if(clientSock == -1)
    {
        perror("accept failed");
        return false;
    }

    int emptyIndex = -1;
    for(int i = 0; i < maxClients; ++i)
    {
        if(clientDatas[i].fd < 0)
        {
            emptyIndex = i;
            break;
        }
    }

    if(emptyIndex < 0)
    {
        printf("too many connections\n");
        close(clientSock);
        return false;
    }

    if(!setNonBlock(clientSock))
    {
        printf("setNonBlock failed\n");
        close(clientSock);
        return false;
    }
    epoll_event ev;
    ev.events = (EPOLLIN | EPOLLOUT);    
    ev.data.ptr = &clientDatas[emptyIndex];    
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSock, &ev) == -1)
    {
        perror("epoll_ctl failed");
        close(clientSock);
        return false;
    }

    clientDatas[emptyIndex].fd = clientSock;
    clientDatas[emptyIndex].buffer.init(BUF_SIZE);

    return true;
}

void processPollIn(ClientData *pData)
{
    TransferRingBuffer &tBuf = pData->buffer;
    
    void *data;
    int size;

    if(tBuf.startWrite(data, size))
    {
        int rd = read(pData->fd, data, size);        

        if(rd <= 0)
        {
            if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
            {
                if(rd == 0 && errno == 0)
                {
                    printf("client disconnected\n");
                }
                else
                {
                    printf("read failed: %s\n", strerror(errno));
                }                
                close(pData->fd);
                pData->fd = -1;
                return;
            }
        }
        else
        {            
            tBuf.endWrite(rd);
        }
    }
}


void processPollOut(ClientData *pData)
{
    TransferRingBuffer &tBuf = pData->buffer;
    
    void *data;
    int size;

    if(tBuf.startRead(data, size))
    {
        if(withCheck)
        {
            checkBuffer(data, size);
        }

        int wr = write(pData->fd, data, size);
        if(wr <= 0)
        {
            if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
            {
                printf("write failed: %s\n", strerror(errno));
                close(pData->fd);
                pData->fd = -1;                
                return;
            }
        }
        else
        {
            tBuf.endRead(wr);
        }
    }
}

static void closeClients()
{
    for(int i = 0; i < maxClients; ++i)
    {
        if(clientDatas[i].fd >= 0)
        {
            close(clientDatas[i].fd);
            clientDatas[i].fd = -1;
        }
    }
}

static void closeAll()
{
    closeClients();
    if(serverSockFd > 0)
    {
        close(serverSockFd);
    }
}

static bool serverEpoll()
{
    serverSockFd = socketListen(7000);
    if(serverSockFd <= 0)
    {
        return false;
    }

    int epollFd = epoll_create1(0);

    if(epollFd == -1)
    {
        perror("epoll_create1 failed");
        closeAll();
        return false;
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serverSockFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSockFd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        close(serverSockFd);
        return false;
    }

    epoll_event events[maxClients + 1];

    while(true)
    {
        int nFds = epoll_wait(epollFd, events, maxClients + 1, -1);
        if(nFds == -1)
        {
            perror("epoll_wait failed");
            closeAll();
            return false;
        }

        for(int i = 0; i < nFds; ++i)
        {
            if(events[i].data.fd == serverSockFd)
            {
                if(!processAccept(epollFd))
                {
                    closeAll();
                    return false;
                }
            }
            else
            {
                if((events[i].events & POLLIN) != 0)
                {
                    processPollIn((ClientData*)events[i].data.ptr);
                }
                if((events[i].events & POLLOUT) != 0)
                {
                    processPollOut((ClientData*)events[i].data.ptr);
                }
            }
        }
    }
}


int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("usage: server_epoll [check]\n");
    }

    signal(SIGINT, sig_int_handler);

    if(argc >= 2 && strcmp(argv[1], "check") == 0)
    {
        withCheck = true;
        printf("running with check\n");
    }

    serverEpoll();

    return 0;
}
