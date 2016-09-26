#ifndef SERVER_EXECUTOR_H
#define SERVER_EXECUTOR_H

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

#include <ProcessResult.h>
#include <NetworkUtils.h>
#include <ServerContext.h>

class ServerExecutor: public Executor {
public:
	~ServerExecutor()
	{
		stop();
	}

	int init(ServerParameters &params, ServerContext &context, int serverSocketFd)
	{
		stop();
		this->serverSocketFd = serverSocketFd;		
		ctx = &context;
		return 0;
	}

	void stop()
	{
		if(serverSocketFd > 0)
		{
			close(serverSocketFd);
			serverSocketFd = -1;
		}
	}


	int process(int fd, int events, ProcessResult &result) override
	{
		if(fd != serverSocketFd)
		{
			ctx->log->error("invalid file\n");
			result.action = ProcessResult::Action::none;
			return -1;
		}

		struct sockaddr_in address;
		socklen_t addrlen = sizeof(address);

        int clientSockFd = accept4(serverSocketFd, (struct sockaddr *)&address, (socklen_t*)&addrlen, SOCK_NONBLOCK);

		if(clientSockFd == -1)
		{
			ctx->log->error("accept failed: %s", strerror(errno));
			result.action = ProcessResult::Action::shutdown;
			return -1;
		}

		result.action = ProcessResult::Action::createExecutor;
		result.addFd = clientSockFd;
        result.addFdEvents = EPOLLIN;

		return 0;
	}

	int getFds(int *fds) override
	{
		return 0;
	}
protected:
	int serverSocketFd = -1;

	ServerContext *ctx;
};

#endif
