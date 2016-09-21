
struct ProcessResult {
	enum class Result { };

};


class ServerExecutor: Executor {
public:
	ProcessResult process(int fd, int events) override
	{
		if(fd != serverSocketFd)
		{
			//action none
			//status invalid
		}

		struct sockaddr_in address;
		socklen_t addrlen = sizeof(address);

		int clientSockFd = accept(serverSockFd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

		if(clientSockFd == -1)
		{
			perror("accept failed");
			//action none
			//status error
		}

		if(!setNonBlock(clientSockFd))
		{
			printf("setNonBlock failed\n");
			close(clientSockFd);
			//action none
			//status error
		}

		result.action = createExecutor;
		result.executorType = ClientExecutor;
		result.fd = clientSockFd;
		result.events = (POLLIN | POLLOUT);

		return result;
	}


protected:
	int serverSocketFd;
};

