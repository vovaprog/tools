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
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <NetworkUtils.h>
#include <TransferRingBuffer.h>
#include <IncPerSecond.h>

static int sockfd = -1;
static const int BUF_SIZE = 1000000;
static bool withCheck = false;
static char buf[BUF_SIZE];
char socketName[300] = {};

static void sig_int_handler(int i)
{
    printf("sig int handler\n");

    if(sockfd > 0)
    {
        close(sockfd);
    }
    if(socketName[0] != 0)
    {
        unlink(socketName);
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


void readFromFile(int fd)
{
    IncPerSecond incCounter(1000000);

    while(true)
    {
        int rd = read(fd, buf, BUF_SIZE);

        if(rd <= 0)
        {
            if(rd == 0)
            {
                printf("client disconnected\n");
                close(fd);
                return;
            }
            else
            {
                printf("read failed: %s\n", strerror(errno));
                close(fd);
                return;
            }
        }

        if(withCheck)
        {
            checkBuffer(buf, rd);
        }

        incCounter.addAndPrint(rd);
    }
}


static void* clientThreadEntry(void *arg)
{
    int clientSocket = *static_cast<int*>(arg);
    free(arg);

    readFromFile(clientSocket);

    return nullptr;
}


static bool server()
{
    char workingDir[300];

    if(getcwd(workingDir, 300) == NULL)
    {
        printf("cwd failed\n");
        return -1;
    }

    if(snprintf(socketName, 300, "%s/my_unix_socket", workingDir) >= 300)
    {
        printf("socket name too long\n");
        return -1;
    }

    sockfd = socketListenUnix(socketName);
    if(sockfd <= 0)
    {
        return false;
    }

    struct sockaddr_un cli_addr;
    socklen_t clilen = sizeof(sockaddr_un);

    while(true)
    {
        int clientSocket = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);

        if(clientSocket < 0)
        {
            printf("accept failed\r\n");
        }
        else
        {
            pthread_t th;
            int *pClientSocket = (int*)malloc(sizeof(int));
            *pClientSocket = clientSocket;

            pthread_create(&th, nullptr, clientThreadEntry, pClientSocket);
        }
    }

    return true;
}


int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("usage: server_read_unix [check]\n");
    }

    signal(SIGINT, sig_int_handler);

    if(argc >= 2 && strcmp(argv[1], "check") == 0)
    {
        withCheck = true;
        printf("running with check\n");
    }

    server();

    return 0;
}
