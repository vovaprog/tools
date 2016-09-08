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


static int client(const char *addr, bool withCheck)
{
    sock = socketConnect(addr, 7000);

    if(sock <= 0)
    {
        return -1;
    }

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
                int wr = write(sock, data, size);
                if(wr <= 0)
                {
                    printf("write failed: %s\n", strerror(errno));
                    close(sock);
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
            int wr = write(sock, buf, BUF_SIZE);
            if(wr <= 0)
            {
                printf("write failed: %s\n", strerror(errno));
                close(sock);
                return -1;
            }
            incCounter.addAndPrint(wr);
        }
    }

    close(sock);

    return 0;
}


int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("usage: client [ ip [check] ]\n");
    }

    signal(SIGINT, sig_int_handler);

    const char * addr = "127.0.0.1";
    bool withCheck = false;

    if(argc >= 2)
    {
        addr = argv[1];
    }
    if(argc >= 3 && strcmp(argv[2], "check") == 0)
    {
        withCheck = true;
        printf("running with check\n");
    }

    client(addr, withCheck);

    return 0;
}



