#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <IncPerSecond.h>

const int BUF_SIZE = 10000;
int buf[BUF_SIZE];
const char *fifoName = "./my_fifo";

volatile sig_atomic_t runFlag = 1;


static void sig_int_handler(int i)
{
    printf("sig int handler\n");

    runFlag = 0;
}


void readFromFile(int fd)
{
    IncPerSecond incCounter(1000000);

    while(runFlag)
    {
        int rd = read(fd, buf, BUF_SIZE);

        if(rd <= 0)
        {
            if(rd == 0)
            {
                printf("client disconnected\n");
            }
            else
            {
                printf("read failed: %s\n", strerror(errno));
            }
            break;
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
    unlink(fifoName);
    
    //mode is modified by the process's umask in the usual way: the permissions of the created file are (mode & ~umask).
    if(mkfifoat(AT_FDCWD, fifoName, 0666) != 0)
    {
        perror("mkfifo failed\n");
        return -1;
    }
  
    int fd = open(fifoName, O_WRONLY);

    if(fd < 0)
    {
        perror("open failed");
        return -1;
    }

    IncPerSecond incCounter(1000000);

    while(runFlag)
    {
        int wr = write(fd, buf, BUF_SIZE);
        if(wr <= 0)
        {
            printf("write failed: %s\n", strerror(errno));
            break;
        }
        incCounter.addAndPrint(wr);
    }

    close(fd);

    if(unlink(fifoName) != 0)
    {
        perror("unlink failed\n");
        return -1;
    }
   
    return 0;
}


int readFifo()
{    
    int fd = open(fifoName, O_RDONLY);

    if(fd < 0)
    {
        perror("open failed");
        return -1;
    }

    readFromFile(fd);

    close(fd);

    return 0;
}

int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("usage: fifo_speed r|w\nstart with w first to create fifo\n");
        return -1;
    }

    signal(SIGINT, sig_int_handler);

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





