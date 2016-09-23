#ifndef SERVER_H
#define SERVER_H

#include <ClientExecutor.h>
#include <ServerExecutor.h>
#include <ProcessResult.h>

class Server {
public:
	struct Parameters {
		int maxClients = 0;
		int port = 0;
	};

	~Server()
	{
		destroy();
	}

	void destroy()
	{
		if(clientExecutors!=nullptr)
		{
			delete[] clientExecutors;
			clientExecutors = nullptr;
		}
		if(des!=nullptr)
		{
			delete[] des;
			des = nullptr;
		}
		if(events!=nullptr)
		{
			delete[] events;
			events = nullptr;
		}
		if(epollFd > 0)
		{
			close(epollFd);
			epollFd = -1;
		}
	}

	int initDataStructs(Parameters &params)
	{
		MAX_CLIENTS = params.maxClients;
		MAX_EVENTS = MAX_CLIENTS * 2 + 1;

		clientExecutors = new ClientExecutor[MAX_CLIENTS];
		des = new DescriptorExecutor[MAX_EVENTS];
		events = new epoll_event[MAX_EVENTS];

		emptyExecutors.reserve(MAX_CLIENTS);
		emptyD2es.reserve(MAX_EVENTS);

		for(int i = MAX_CLIENTS - 1; i >= 0; --i)
		{
			emptyExecutors.push(i);
		}
		for(int i = MAX_EVENTS - 1; i >= 0; --i)
		{
			emptyD2es.push(i);
		}
		return 0;
	}

	int addPollFd(int fd, uint32_t events, Executor *pExecutor, int executorIndex)
	{
		if(emptyD2es.size() == 0)
		{
			printf("too many connections\n");
			return -1;
		}

		int index = emptyD2es.top();

		epoll_event ev;
		ev.events = events;
		ev.data.ptr = executor;
		if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			perror("epoll_ctl failed");
			return -1;
		}

		des[index].fd = fd;
		des[index].pExecutor = pExecutor;
		des[index].executorIndex = executorIndex;

		emptyD2es.pop();

		return 0;
	}



	int run(Parameters &params)
	{
		serverSockFd = socketListen(params.port);
		if(serverSockFd < 0)
		{
			return -1;
		}


		epollFd = epoll_create1(0);
		if(epollFd == -1)
		{
			perror("epoll_create1 failed");
			destroy();
			return -1;
		}


		if(addPollFd(serverSockFd, EPOLLIN, &serverExecutor, -1) != 0)
		{
			destroy();
			return -1;
		}


		if(initDataStructs(params) != 0)
		{
			destroy();
			return -1;
		}


		ProcessResult result;

		while(true)
		{
			int nEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
			if(nEvents == -1)
			{
				perror("epoll_wait failed");
				destroy();
				return -1;
			}

			for(int i = 0; i < nEvents; ++i)
			{
				DescriptorExecutor *pDe = events[i].data.ptr;

				Executor *pExecutor = pDe->pExecutor;

				result.reset();
				result.pDe = pDe;

				pExecutor->process(d2e->fd, events[i].events, result);

				handleResult(result);
			}
		}
	}

	int handleResult_createExecutor(ProcessResult result)
	{
		if(emptyExecutors.size() == 0 || emptyD2e.size() == 0)
		{
			printf("too many connections!");
			return -1;
		}
		int executorIndex = emptyExecutors.top();
		int d2eIndex = emptyD2e.top();

		clientExecutors[index].init(result.fd);

		if(addPollFd(result.fd, result.events, &clientExecutors[executorIndex]) != 0)
		{
			return -1;
		}

		emptyExecutors.pop();

		return 0;
	}

	int handleResult_removeExecutor(ProcessResult &result)
	{
		result.pDe->pExecutor->close();

		if(result.pDe.executorIndex >= 0)
		{
			emptyExecutors.push(i);
		}

		int ret = 0;
		bool found = false;

		const int MAX_FDS_OF_EXECUTOR = 2;
		int fdsOfExecutor[MAX_FDS_OF_EXECUTOR] = {-1, -1};
		int numberOfFds = 0;

		result.pExecutor->getFds(fdsOfExecutor, numberOfFds);

		for(int i=0;i<numberOfFds;++i)
		{
			if(tryGetValue(fd2Des,fdsOfExecutor[i],desIndex))
			{
				des[desIndex].fd = -1;
				des[desIndex].pExecutor = nullptr;
				des[desIndex].executorIndex = -1;
				emptyDes.push(desIndex);
				close(fdsOfExecutor[i]);
			}
			else
			{
				printf("des not found\n");
			}
		}

		return 0;
	}

	int editPoll(ProcessResult &result)
	{
		int ret = 0;

		if(result.addFd > 0)
		{
			ret = addPollFd(result.addFd, result.addFdEvents, result.pExecutor);
		}
		if(result.editFd > 0)
		{
			epoll_event ev;
			ev.events = result.editFdEvents;
			ev.data.ptr = result.pDe;
			if(epoll_ctl(epollFd, EPOLL_CTL_ADD, result.editFd, &ev) == -1)
			{
				perror("epoll_ctl failed");
				ret = -1;
			}
		}
		return ret;
	}

	int handleResult(ProcessResult &result)
	{
		if(result.action == createExecutor)
		{
			return createExecutor(result);
		}
		else if(result.action == removeExecutor)
		{
			return removeExecutor(result);
		}
		else if(result.action == changePoll)
		{
			return editPoll(result);
		}
		else
		{
			printf("unknown action\n");
			return -1;
		}
	}

protected:
	struct DescriptorExecutor
	{
		int fd = -1;
		Executor *pExecutor = nullptr;
	};

	ClientExecutor *clientExecutors = nullptr;
	DescriptorExecutor *des = nullptr;
	epoll_event *events = nullptr;

	std::stack<int, std::vector<int>> emptyExecutors;
	std::stack<int, std::vector<int>> emptyD2es;

	int epollFd = -1;
};

#endif
