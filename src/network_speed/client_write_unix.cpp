#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <NetworkUtils.h>
#include <IncPerSecond.h>
#include <TransferRingBuffer.h>

static int sock = -1;
static const int BUF_SIZE = 1000000;
static char buf[BUF_SIZE];

static void sig_int_handler(int i)
{
    printf("sig int handler\n");

    if(sock > 0)
    {
        close(sock);
    }

    exit(-1);
}


int writeToFile(int fd, bool withCheck)
{
    IncPerSecond incCounter(1000000);

    int charCounter = 0;

    TransferRingBuffer sendBuf(BUF_SIZE);

    if(withCheck)
    {
        while(true)
        {
            void *data;
            int size;

            if(sendBuf.startWrite(data, size))
            {
                for(int i = 0; i < size; ++i)
                {
                    ((unsigned char*)data)[i] = charCounter;
                    charCounter = ((charCounter + 1) & 0xff);
                }
                sendBuf.endWrite(size);
            }

            if(sendBuf.startRead(data, size))
            {
                int wr = write(fd, data, size);
                if(wr <= 0)
                {
                    printf("write failed: %s\n", strerror(errno));
                    close(fd);
                    return -1;
                }
                else
                {
                    sendBuf.endRead(wr);
                }

                incCounter.addAndPrint(wr);
            }
        }
    }
    else
    {
        while(true)
        {
            int wr = write(fd, buf, BUF_SIZE);
            if(wr <= 0)
            {
                printf("write failed: %s\n", strerror(errno));
                close(fd);
                return -1;
            }
            incCounter.addAndPrint(wr);
        }
    }


}

static int client(bool withCheck)
{
    char workingDir[300], socketName[300];

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

    sock = socketConnectUnix(socketName);

    if(sock <= 0)
    {
        return -1;
    }

    writeToFile(sock, withCheck);

    close(sock);

    return 0;
}


int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("usage: client_write_unix [check]\n");
    }

    bool withCheck;

    if(argc >= 2 && strcmp(argv[1], "check") == 0)
    {
        withCheck = true;
        printf("running with check\n");
    }
   
    signal(SIGINT, sig_int_handler);

    client(withCheck);

    return 0;
}



