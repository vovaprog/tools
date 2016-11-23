#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#include <IncPerSecond.h>

const int BUF_SIZE = 10000;
int buf[BUF_SIZE];



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

        incCounter.addAndPrint(rd);
    }
}

int createFifoName(char *buf, int bufSize)
{
    char workingDir[300];

    if(getcwd(workingDir, 300) == NULL)
    {
        printf("cwd failed\n");
        return -1;
    }

    if(snprintf(buf, bufSize, "%s/my_fifo", workingDir) >= bufSize)
    {
        printf("socket name too long\n");
        return -1;
    }

    return 0;
}


int writeFifo()
{
    char fifoName[300];

    if(createFifoName(fifoName, 300) != 0)
    {
        return -1;            
    }

    int fd = open(fifoName, O_WRONLY);

    if(fd < 0)
    {
        perror("open failed");
        return -1;
    }

    IncPerSecond incCounter(1000000);

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

    close(fd);

    return 0;
}



int readFifo()
{
    char fifoName[300];

    if(createFifoName(fifoName, 300) != 0)
    {
        return -1;
    }

    unlink(fifoName);

    mkfifo(fifoName, 0666);

    int fd = open(fifoName, O_RDONLY);

    if(fd < 0)
    {
        perror("open failed");
        return -1;
    }

    readFromFile(fd);

    unlink(fifoName);

    return 0;
}

int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("usage: fifo_speed r|w\nstart with r first to create fifo\n");
    }

    if(strcmp(argv[1], "r") == 0)
    {
        readFifo();
    }
    else
    {
        writeFifo();
    }
    
    return 0;
}





