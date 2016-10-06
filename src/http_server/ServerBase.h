#ifndef SERVER_BASE_H
#define SERVER_BASE_H

#include <openssl/ssl.h>

#include <Log.h>
#include <ExecutorType.h>

class ServerBase
{
public:

    virtual int createRequestExecutor(int fd, ExecutorType execType) = 0;

    Log *log = nullptr;
    SSL_CTX* globalSslCtx = nullptr;
};

#endif

