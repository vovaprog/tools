#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <stack>
#include <unordered_map>
#include <signal.h>

#include <ClientExecutor.h>
#include <ServerExecutor.h>
#include <ProcessResult.h>
#include <ServerParameters.h>
#include <ServerContext.h>
#include <LogStdout.h>
#include <ExecutorData.h>
#include <PollData.h>

class Server {
public:
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

    int initDataStructs(ServerParameters &params)
	{
		MAX_CLIENTS = params.maxClients;
		MAX_EVENTS = MAX_CLIENTS * 2 + 1;

		execDatas = new ExecutorData[MAX_CLIENTS];
		pollDatas = new PollData[MAX_EVENTS];
		events = new epoll_event[MAX_EVENTS];

		for(int i = MAX_CLIENTS - 1; i >= 0; --i)
		{
			emptyExecutors.push(i);
		}
		for(int i = MAX_EVENTS - 1; i >= 0; --i)
		{
			emptyDes.push(i);
		}
		/*for(int i=0;i<MAX_CLIENTS;++i)
        {
			clientExecutors[i].init(params, ctx);
		}*/
		return 0;
	}

	int addPollFd(int fd, uint32_t events, int execIndex)
	{
		if(emptyPollDatas.size() == 0)
		{
			ctx.log->error("too many connections\n");
			return -1;
		}

		int pollIndex = emptyPollDatas.top();

		epoll_event ev;
		ev.events = events;
		ev.data.ptr = &pollDatas[pollIndex];
		if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			perror("epoll_ctl failed");
			return -1;
		}

		pollDatas[index].fd = fd;
		pollDatas[index].executorDataIndex = execIndex;

		if(fd == execDatas[execIndex].fd0)
		{
			execDatas[execIndex].pollIndexFd0 = pollIndex;
		}
		else
		{
			execDatas[execIndex].pollIndexFd1 = pollIndex;
		}

		emptyPollDatas.pop();

		return 0;
	}


	int openServerSocket(ServerParameters &params)
	{
		int serverSockFd = socketListen(params.port);
		if(serverSockFd < 0)
		{
			return -1;
		}

		int execIndex = emptyExecDatas.top();
		execDatas[execIndex].fd0 = serverSockFd;
		execDatas[execIndex].pExecutor = &serverExecutor;

		int pollIndex = emptyPollDatas.top();
		pollDatas[pollIndex].fd = serverSockFd;
		pollDatas[pollIndex].executorDataIndex = execIndex;

		if(addPollFd(execDatas[execIndex].fd0, EPOLLIN, &pollDatas[pollIndex], -1) != 0)
		{
			close(serverSockFd);
			return -1;
		}

		emptyExecDatas.pop();
		emptyPollDatas.pop();

		return 0;
	}


    int run(ServerParameters &params)
	{
		strcpy(ctx.fileNameBuffer, params.rootFolder);
		strcat(ctx.fileNameBuffer, "/");
		ctx.rootFolderLength = strlen(ctx.fileNameBuffer);

		logStdout.init(params.logLevel);
		ctx.log = &logStdout;


        signal(SIGPIPE, SIG_IGN);


		if(initDataStructs(params) != 0)
		{
			destroy();
			return -1;
		}


		epollFd = epoll_create1(0);
		if(epollFd == -1)
		{
			perror("epoll_create1 failed");
			destroy();
			return -1;
		}


		if(openServerSocket(params) != 0)
		{
			destroy();
			return -1;
		}


		ProcessResult result;

		while(epollFd > 0)
		{
            printStats();

			int nEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
			if(nEvents == -1)
			{
                if(errno == EINTR)
                {
                    continue;
                }
                else
                {
                    perror("epoll_wait failed");
                    destroy();
                    return -1;
                }
			}

			for(int i = 0; i < nEvents; ++i)
			{
				PollData *pollData = static_cast<DescriptorExecutor*>(events[i].data.ptr);
				ExecutorData &execData = execDatas[pollData->executorDataIndex];

				result.reset();
				result.pollData = pollData;

				execData.pExecutor->process(execData, pollData->fd, events[i].events, result);

				handleResult(result);
			}
		}

		return 0;
	}

	int handleResult_createExecutor(ProcessResult result)
	{
		if(emptyExecDatas.size() == 0 || emptyPollDatas.size() == 0)
		{
			ctx.log->error("too many connections!\n");
			return -1;
		}
		int execIndex = emptyExecDatas.top();

		execDatas[execIndex].pExecutor = &requestExecutor;
		requestExecutor.up(execDatas[execIndex], result.addFd);

		if(addPollFd(result.addFd, result.addFdEvents, execIndex) != 0)
		{
			return -1;
		}

		emptyExecDatas.pop();

		return 0;
	}

	int removeExecutor(int execIndex)
	{
		if(execDatas[execIndex].pollIndexFd0 >=0)
		{
			emptyPollDatas.push(execDatas[execIndex].pollIndexFd0);
		}
		if(execDatas[execIndex].pollIndexFd1 >=0)
		{
			emptyPollDatas.push(execDatas[execIndex].pollIndexFd1);
		}

		execDatas[execIndex].down();

		emptyExecDatas.push(execIndex);

		return 0;
	}

	int handleResult_removeExecutor(ProcessResult &result)
	{
		int execIndex = result.pollData->executorDataIndex;

		return removeExecutor(execIndex);
	}

	int handleResult_editPoll(ProcessResult &result)
	{
		int ret = 0;

		if(result.addFd > 0)
		{
			ret = addPollFd(result.addFd, result.addFdEvents, result.pollData->executorDataIndex);
		}
		if(result.editFd > 0)
		{
			epoll_event ev;
			ev.events = result.editFdEvents;
			ev.data.ptr = result.pollData;
			if(epoll_ctl(epollFd, EPOLL_CTL_MOD, result.editFd, &ev) == -1)
			{
				ctx.log->error("epoll_ctl failed: %s\n", strerror(errno));
				ret = -1;
			}
		}
		return ret;
	}

	int handleResult_setFileExecutor(ProcessResult &result)
	{
		int execIndex = result.pollData->executorDataIndex;
		execDatas[execIndex].pExecutor = &fileExecutor;
		if(fileExecutor.up(execDatas[execIndex])!=0)
		{
			removeExecutor(execIndex);
			return -1;
		}
		return 0;
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

		case ProcessResult::Action::setFileExecutor:
			return handleResult_setFileExecutor(result);

        case ProcessResult::Action::none:
            return 0;

		default:
			ctx.log->error("unknown action\n");
			return -1;
		}
	}

protected:

    void printStats()
    {
		ctx.log->debug("num of connections: %d\n", MAX_CLIENTS - (int)emptyExecDatas.size() - 1);
    }

	ServerExecutor serverExecutor;
	RequestExecutor requestExecutor;
	FileExecutor fileExecutor;
	UwsgiExecutor uwsgiExecutor;

	ServerContext ctx;
	LogStdout logStdout;

	ExecutorData *execDatas = nullptr;
	PollData *pollDatas = nullptr;
	epoll_event *events = nullptr;

	std::stack<int, std::vector<int>> emptyExecDatas;
	std::stack<int, std::vector<int>> emptyPollDatas;

	int epollFd = -1;

	int MAX_CLIENTS = 0, MAX_EVENTS = 0;

};

#endif
