#ifndef UTIL_H
#define UTIL_H

int readBytes(int fd, char *buf, int numberOfBytes);
int writeBytes(int fd, char *buf, int numberOfBytes);

int socketConnect(const char *address, int port);
int socketListen(int port);

int socketConnectUnix(const char *socketName);
int socketListenUnix(const char *socketName);

bool setNonBlock(int fd);

#endif

