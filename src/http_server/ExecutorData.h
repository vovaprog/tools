#ifndef EXECUTOR_DATA_H
#define EXECUTOR_DATA_H

#include <TransferRingBuffer.h>

#include <sys/types.h>
#include <openssl/ssl.h>

class Executor;

struct ExecutorData
{
    ~ExecutorData()
    {
        down();
    }

    void down();

    static const int REQUEST_BUFFER_SIZE = 10000;

    enum class State
    {
        invalid, readRequest, sendResponse, sendFile,
        forwardRequest, forwardResponse, forwardResponseOnlyWrite,
        sslHandshake
    };

    int index = -1;

    Executor *pExecutor = nullptr;

    State state = State::invalid;

    int fd0 = -1;
    int fd1 = -1;
    int pollIndexFd0 = -1;
    int pollIndexFd1 = -1;

    long long int bytesToSend = 0;
    off_t filePosition = 0;

    TransferRingBuffer buffer;

    int port = 0;

    SSL *ssl = nullptr;

    long long int lastProcessTime = 0;
    bool removeOnTimeout = true;
};

#endif
