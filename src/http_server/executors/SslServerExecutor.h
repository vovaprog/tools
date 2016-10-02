#ifndef SSL_SERVER_EXECUTOR_H
#define SSL_SERVER_EXECUTOR_H

#include <sys/sendfile.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
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
#include <sys/epoll.h>
#include <fcntl.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <ProcessResult.h>
#include <NetworkUtils.h>
#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>
#include <Log.h>

class SslServerExecutor: public Executor
{
public:
	int init(PollLoopBase *loop) override
    {
		this->loop = loop;
        log = loop->log;
        return 0;
    }

    int up(ExecutorData &data) override
    {
        data.fd0 = socketListen(data.port);
        if(data.fd0 < 0)
        {
            return -1;
        }

        if(loop->addPollFd(data, data.fd0, EPOLLIN) != 0)
        {
            return -1;
        }

        return 0;
    }

	static int initSsl(Log *log, SSL_CTX* &globalSslCtx)
	{
		SSL_load_error_strings ();
		SSL_library_init ();

		globalSslCtx = SSL_CTX_new (SSLv23_method());
		if(globalSslCtx == NULL)
		{
			log->error("SSL_CTX_new failed\n");
			return -1;
		}

		//BIO* errBio = BIO_new_fd(2, BIO_NOCLOSE);

		if(SSL_CTX_use_certificate_file(globalSslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
		{
			log->error("SSL_CTX_use_certificate_file failed\n");
			SSL_CTX_free(globalSslCtx);
			return -1;
		}

		if(SSL_CTX_use_PrivateKey_file(globalSslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
		{
			log->error("SSL_CTX_use_PrivateKey_file failed\n");
			SSL_CTX_free(globalSslCtx);
			return -1;
		}

		if(SSL_CTX_check_private_key(globalSslCtx) <= 0)
		{
			log->error("SSL_CTX_check_private_key\n");
			SSL_CTX_free(globalSslCtx);
			return -1;
		}

		log->info("ssl inited\n");

		return 0;
	}



    ProcessResult process(ExecutorData &data, int fd, int events) override
    {
        if(fd != data.fd0)
        {
            log->error("invalid file\n");
            return ProcessResult::ok;
        }

        struct sockaddr_in address;
        socklen_t addrlen = sizeof(address);

        int clientSockFd = accept4(data.fd0, (struct sockaddr *)&address, (socklen_t*)&addrlen, SOCK_NONBLOCK);

        if(clientSockFd == -1)
        {
            log->error("accept failed: %s", strerror(errno));
            return ProcessResult::shutdown;
        }

		loop->createRequestExecutor(clientSockFd, ExecutorType::requestSsl);

        return ProcessResult::ok;
    }
};

#endif
