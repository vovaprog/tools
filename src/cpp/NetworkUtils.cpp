#include "NetworkUtils.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>


int readBytes(int fd, char *buf, int numberOfBytes)
{
    int bytesRead = 0;
    while(bytesRead < numberOfBytes)
    {
        int curBytes = read(fd, buf + bytesRead, numberOfBytes - bytesRead);
        if(curBytes < 0)
        {
            return -1;
        }
        else if(curBytes == 0)
        {
            return bytesRead;
        }
        bytesRead += curBytes;
    }
    return bytesRead;
}

int writeBytes(int fd, char *buf, int numberOfBytes)
{
    int bytesWritten = 0;
    while(bytesWritten < numberOfBytes)
    {
        int curBytes = write(fd, buf + bytesWritten, numberOfBytes - bytesWritten);
        if(curBytes <= 0)
        {
            return -1;
        }
        bytesWritten += curBytes;
    }
    return bytesWritten;
}

int socketConnect(const char *address, int port)
{
    struct sockaddr_in remoteaddr;
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_addr.s_addr = inet_addr(address);
    remoteaddr.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        printf("socket failed!\r\n");
        return -1;
    }

    if(connect(sock, (struct sockaddr*)&remoteaddr, sizeof(remoteaddr)) != 0)
    {
        printf("connect failed!\r\n");
        close(sock);
        return -1;
    }

    return sock;
}

int socketListen(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0)
    {
        printf("socket failed\n");
        return -1;
    }

    int enable = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
    {
        printf("setsockopt failed\n");
        close(sockfd);
        return -1;
    }

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in)) != 0)
    {
        printf("bind failed: %s\r\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    if(listen(sockfd, 1000) != 0) //length of queue of pending connections
    {
        printf("listen failed\r\n");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

bool setNonBlock(int fd)
{
    int flags; 
    
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) 
    { 
        return false; 
    } 
    
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) 
    { 
        return false; 
    } 

    return true;    
}

