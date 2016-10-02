#ifndef EXECUTOR_DATA_H
#define EXECUTOR_DATA_H

#include <openssl/ssl.h>

#include <TransferRingBuffer.h>

class Executor;

struct ExecutorData
{
    ~ExecutorData()
    {
        down();
    }

    void down()
    {
        if(fd0 > 0)
        {
            close(fd0);
            fd0 = -1;
        }
        if(fd1 > 0)
        {
            close(fd1);
            fd1 = -1;
        }
        pollIndexFd0 = -1;
        pollIndexFd1 = -1;

        state = State::invalid;
        pExecutor = nullptr;

        bytesToSend = 0;
        filePosition = 0;

        buffer.clear();

		if(ssl != nullptr)
		{
			SSL_shutdown(ssl);
			SSL_free(ssl);
			ssl = nullptr;
		}

        return;
    }

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
};

#endif
