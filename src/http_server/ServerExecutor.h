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
#include <Executor.h>
#include <ExecutorData.h>

class ServerExecutor: public Executor {
public:

	int up(ExecutorData &data) override
	{
		return 0;
	}

	int process(ExecutorData &data, int fd, int events, ProcessResult &result) override
	{
		if(fd != data.fd0)
		{
			data.ctx->log->error("invalid file\n");
			result.action = ProcessResult::Action::none;
			return -1;
		}

		struct sockaddr_in address;
		socklen_t addrlen = sizeof(address);

		int clientSockFd = accept4(data.fd0, (struct sockaddr *)&address, (socklen_t*)&addrlen, SOCK_NONBLOCK);

		if(clientSockFd == -1)
		{
			data.ctx->log->error("accept failed: %s", strerror(errno));
			result.action = ProcessResult::Action::shutdown;
			return -1;
		}

		result.action = ProcessResult::Action::createExecutor;
		result.addFd = clientSockFd;
        result.addFdEvents = EPOLLIN;

		return 0;
	}
};

#endif
