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

#include <NetworkUtils.h>
#include <TransferRingBuffer.h>

static int serverSockFd = -1;
static const int BUF_SIZE = 1000000;
static bool withCheck = false;
const int maxPollFds = 20;
TransferRingBuffer clientBuffers[maxPollFds];


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

void processAccept(pollfd *pollFds, TransferRingBuffer *clientBuffers, int maxPollFds, int &pollFdsSize)
{
    int serverSockFd = pollFds[0].fd;

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    int clientSockFd = accept(serverSockFd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    if(clientSockFd < 0)
    {
        printf("accept failed\r\n");
        close(serverSockFd);
        exit(-1);
    }

    printf("client connected   ip: %s   port: %d\n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

    int emptyIndex = -1;
    for(int i = 1; i < maxPollFds; ++i)
    {
        if(pollFds[i].fd < 0)
        {
            emptyIndex = i;
            break;
        }
    }

    if(emptyIndex > 0)
    {
        pollFds[emptyIndex].fd = clientSockFd;
        pollFds[emptyIndex].events = (POLLIN | POLLOUT);
        if(emptyIndex >= pollFdsSize)
        {
            pollFdsSize = emptyIndex + 1;
        }
        clientBuffers[emptyIndex].init(BUF_SIZE);
    }
    else
    {
        printf("too many connections\n");
        close(clientSockFd);
    }
}


void processPollIn(pollfd *pollFds, TransferRingBuffer *clientBuffers, int ind)
{
    int clientSockFd = pollFds[ind].fd;

    TransferRingBuffer &tBuf = clientBuffers[ind];

    void *data;
    int size;

    if(tBuf.startWrite(data, size))
    {
        int rd = read(clientSockFd, data, size);
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
                close(clientSockFd);
                pollFds[ind].fd = -1;
                return;
            }
        }
        else
        {
            tBuf.endWrite(rd);
        }
    }
}


void processPollOut(pollfd *pollFds, TransferRingBuffer *clientBuffers, int ind)
{
    int clientSockFd = pollFds[ind].fd;

    TransferRingBuffer &tBuf = clientBuffers[ind];

    void *data;
    int size;

    if(tBuf.startRead(data, size))
    {
        if(withCheck)
        {
            checkBuffer(data, size);
        }

        int wr = write(clientSockFd, data, size);
        if(wr <= 0)
        {
            if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
            {
                printf("write failed: %s\n", strerror(errno));
                close(clientSockFd);
                pollFds[ind].fd = -1;
                return;
            }
        }
        else
        {
            tBuf.endRead(wr);
        }
    }
}


static bool server_poll()
{
    serverSockFd = socketListen(7000);
    if(serverSockFd <= 0)
    {
        return false;
    }

    struct pollfd pollFds[maxPollFds];

    for(int i = 0; i < maxPollFds; ++i)
    {
        pollFds[i].fd = -1;
    }

    int pollFdsSize = 1;
    pollFds[0].fd = serverSockFd;
    pollFds[0].events = POLLIN;

    while(true)
    {
        int result = poll(pollFds, pollFdsSize, 5000);

        if(result < 0)
        {
            printf("poll failed\r\n");
            close(serverSockFd);
            return false;
        }
        else if(result > 0)
        {
            if(pollFds[0].revents & POLLIN)
            {
                processAccept(pollFds, clientBuffers, maxPollFds, pollFdsSize);
            }

            for(int i = 1; i < pollFdsSize; ++i)
            {
                if(pollFds[i].revents & POLLIN)
                {
                    processPollIn(pollFds, clientBuffers, i);
                }
                if(pollFds[i].revents & POLLOUT)
                {
                    processPollOut(pollFds, clientBuffers, i);
                }
            }
        }
    }
}


int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("usage: server [check]\n");
    }
    
    signal(SIGINT, sig_int_handler);

    if(argc >= 2 && strcmp(argv[1], "check") == 0)
    {
        withCheck = true;
        printf("running with check\n");
    }

    server_poll();

    return 0;
}
