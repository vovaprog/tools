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
#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>
#include <Log.h>

class ServerExecutor: public Executor
{
public:
	int init(PollLoopBase *srv) override
	{
		this->loop = srv;
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

		loop->createRequestExecutor(clientSockFd);

		/*ExecutorData *clientData = srv->createExecutorData();

		if(clientData == nullptr)
		{
			close(clientSockFd);
			return ProcessResult::ok;
		}

		clientData->fd0 = clientSockFd;
		clientData->pExecutor = srv->getExecutor(ExecutorType::request);
		if(srv->addPollFd(*clientData, clientData->fd0, EPOLLIN)!=0)
		{
			srv->removeExecutorData(clientData);
			return ProcessResult::ok;
		}

		clientData->state = ExecutorData::State::readRequest;

		if(clientData->pExecutor->up(*clientData) != 0)
		{
			srv->removeExecutorData(clientData);
			return ProcessResult::ok;
		}*/

		return ProcessResult::ok;
    }

	PollLoopBase *loop = nullptr;
	Log *log = nullptr;
};

#endif
