#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <stack>
#include <unordered_map>

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

		for(int i = MAX_CLIENTS - 1; i >= 0; --i)
		{
			emptyExecutors.push(i);
		}
		for(int i = MAX_EVENTS - 1; i >= 0; --i)
		{
			emptyDes.push(i);
		}
		return 0;
	}

	int addPollFd(int fd, uint32_t events, Executor *pExecutor, int executorIndex)
	{
		if(emptyDes.size() == 0)
		{
			printf("too many connections\n");
			return -1;
		}

		int index = emptyDes.top();

		epoll_event ev;
		ev.events = events;
		ev.data.ptr = &des[index];
		if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			perror("epoll_ctl failed");
			return -1;
		}

		des[index].fd = fd;
		des[index].executorIndex = executorIndex;

		fd2Des[fd] = index;

		emptyDes.pop();

		return 0;
	}



	int run(Parameters &params)
	{
		if(initDataStructs(params) != 0)
		{
			destroy();
			return -1;
		}


		int serverSockFd = socketListen(params.port);
		if(serverSockFd < 0)
		{
			return -1;
		}
		serverExecutor.init(serverSockFd);


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


		ProcessResult result;

		while(epollFd > 0)
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
				DescriptorExecutor *pDe = static_cast<DescriptorExecutor*>(events[i].data.ptr);

				Executor *pExecutor = nullptr;

				if(pDe->fd == serverSockFd)
				{
					pExecutor = &serverExecutor;
				}
				else
				{
					pExecutor = &clientExecutors[pDe->executorIndex];
				}

				result.reset();
				result.pDe = pDe;

				pExecutor->process(pDe->fd, events[i].events, result);

				handleResult(result);
			}
		}

		return 0;
	}

	int handleResult_createExecutor(ProcessResult result)
	{
		if(emptyExecutors.size() == 0 || emptyDes.size() == 0)
		{
			printf("too many connections!");
			return -1;
		}
		int executorIndex = emptyExecutors.top();		

		clientExecutors[executorIndex].init(result.addFd);

		if(addPollFd(result.addFd, result.addFdEvents, &clientExecutors[executorIndex], executorIndex) != 0)
		{
			return -1;
		}

		emptyExecutors.pop();

		return 0;
	}

	int handleResult_removeExecutor(ProcessResult &result)
	{
		int executorIndex = result.pDe->executorIndex;

		int fdsOfExecutor[Executor::MAX_FDS_OF_EXECUTOR];

		int numberOfFds = clientExecutors[result.pDe->executorIndex].getFds(fdsOfExecutor);

		for(int i=0;i<numberOfFds;++i)
		{
			auto iter = fd2Des.find(fdsOfExecutor[i]);
			if(iter != fd2Des.end())
			{
				int desIndex = iter->second;

				des[desIndex].fd = -1;
				des[desIndex].executorIndex = -1;
				emptyDes.push(desIndex);
				fd2Des.erase(iter);
			}
			else
			{
				printf("des not found\n");
			}
		}

		if(executorIndex >= 0)
		{
			clientExecutors[executorIndex].cleanup();
			emptyExecutors.push(executorIndex);
		}

		return 0;
	}

	int handleResult_editPoll(ProcessResult &result)
	{
		int ret = 0;

		if(result.addFd > 0)
		{
			ret = addPollFd(result.addFd, result.addFdEvents, &clientExecutors[result.pDe->executorIndex], result.pDe->executorIndex);
		}
		if(result.editFd > 0)
		{
			epoll_event ev;
			ev.events = result.editFdEvents;
			ev.data.ptr = result.pDe;
			if(epoll_ctl(epollFd, EPOLL_CTL_MOD, result.editFd, &ev) == -1)
			{
				perror("epoll_ctl failed");
				ret = -1;
			}
		}
		return ret;
	}

	int handleResult_shutdown(ProcessResult &result)
	{
		destroy();
		return -1;
	}

	int handleResult(ProcessResult &result)
	{
		switch(result.action){
		case ProcessResult::Action::createExecutor:
			return handleResult_createExecutor(result);
		case ProcessResult::Action::removeExecutor:
			return handleResult_removeExecutor(result);
		case ProcessResult::Action::editPoll:
			return handleResult_editPoll(result);
		case ProcessResult::Action::shutdown:
			return handleResult_shutdown(result);
		default:
			printf("unknown action\n");
			return -1;
		}
	}

protected:

	ServerExecutor serverExecutor;
	ClientExecutor *clientExecutors = nullptr;
	DescriptorExecutor *des = nullptr;
	epoll_event *events = nullptr;

	std::stack<int, std::vector<int>> emptyExecutors;
	std::stack<int, std::vector<int>> emptyDes;
	std::unordered_map<int,int> fd2Des;

	int epollFd = -1;

	int MAX_CLIENTS = 0, MAX_EVENTS = 0;
};

#endif
