#ifndef POLL_LOOP_H
#define POLL_LOOP_H

#include <vector>
#include <stack>
#include <unordered_map>
#include <signal.h>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>

#include <RequestExecutor.h>
#include <FileExecutor.h>
#include <UwsgiExecutor.h>
#include <ServerExecutor.h>
#include <ProcessResult.h>
#include <ServerParameters.h>
#include <ServerContext.h>
#include <LogStdout.h>
#include <ExecutorData.h>
#include <PollData.h>
#include <PollLoopBase.h>


class PollLoop: public PollLoopBase
{
public:
	~PollLoop()
    {
        destroy();
    }

    void destroy()
    {
        if(execDatas != nullptr)
        {
            delete[] execDatas;
            execDatas = nullptr;
        }
        if(pollDatas != nullptr)
        {
            delete[] pollDatas;
            pollDatas = nullptr;
        }
        if(events != nullptr)
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
            emptyExecDatas.push(i);
        }
        for(int i = MAX_EVENTS - 1; i >= 0; --i)
        {
            emptyPollDatas.push(i);
        }
        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            execDatas[i].ctx = &ctx;
			execDatas[i].index = i;
        }
        return 0;
    }

	int addPollFd(ExecutorData &data, int fd, int events) override
    {
        if(emptyPollDatas.size() == 0)
        {
            ctx.log->error("too many connections\n");
            return -1;
        }

		if(fd != data.fd0 && fd != data.fd1)
		{
			ctx.log->error("invalid argument\n");
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

        pollDatas[pollIndex].fd = fd;
		pollDatas[pollIndex].executorDataIndex = data.index;

		if(fd == data.fd0)
        {
			data.pollIndexFd0 = pollIndex;
        }
		else if(fd == data.fd1)
        {
			data.pollIndexFd1 = pollIndex;
        }

        emptyPollDatas.pop();

        return 0;
    }

	int editPollFd(ExecutorData &data, int fd, int events) override
	{
		int pollIndex = -1;
		if(fd == data.fd0)
		{
			pollIndex = data.pollIndexFd0;
		}
		else if(fd == data.fd1)
		{
			pollIndex = data.pollIndexFd1;
		}
		else
		{
			return -1;
		}

		epoll_event ev;
		ev.events = events;
		ev.data.ptr = &pollDatas[pollIndex];
		if(epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) == -1)
		{
			perror("epoll_ctl failed");
			return -1;
		}

		return 0;
	}

	int removePollFd(ExecutorData &data, int fd) override
	{
		if(epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) != 0)
		{
			ctx.log->error("epoll_ctl failed: %s\n", strerror(errno));
			return -1;
		}
		if(fd == data.fd0)
		{
			emptyPollDatas.push(data.pollIndexFd0);
		}
		else if(fd == data.fd1)
		{
			emptyPollDatas.push(data.pollIndexFd1);
		}
		return 0;
	}

    int openServerSocket(ServerParameters &params)
    {
		ExecutorData *pExecData = createExecutorData();

		pExecData->pExecutor = &serverExecutor;

		if(pExecData->pExecutor->up(*pExecData) != 0)
		{
			removeExecutorData(pExecData);
			return -1;
		}

        return 0;
    }

	Executor* getExecutor(ExecutorType execType) override
	{
		switch(execType){
		case ExecutorType::request:
			return &requestExecutor;
		case ExecutorType::file:
			return &fileExecutor;
		case ExecutorType::uwsgi:
			return &uwsgiExecutor;
		case ExecutorType::server:
			return &serverExecutor;
		default:
			return nullptr;
		}
	}

	ExecutorData* createExecutorData() override
	{
		if(emptyExecDatas.size() == 0)
		{
			ctx.log->error("too many connections\n");
			return nullptr;
		}

		int execIndex = emptyExecDatas.top();
		emptyExecDatas.pop();

		return &execDatas[execIndex];
	}

	void removeExecutorData(ExecutorData *execData)
	{
		if(execData->pollIndexFd0 >= 0)
		{
			emptyPollDatas.push(execData->pollIndexFd0);
		}
		if(execData->pollIndexFd1 >= 0)
		{
			emptyPollDatas.push(execData->pollIndexFd1);
		}

		execData->down();

		emptyExecDatas.push(execData->index);
	}

    int run(ServerParameters &params)
    {
		serverExecutor.init(this);
		requestExecutor.init(this);
		fileExecutor.init(this);
		uwsgiExecutor.init(this);

        strcpy(ctx.fileNameBuffer, params.rootFolder);
        strcat(ctx.fileNameBuffer, "/");
        ctx.rootFolderLength = strlen(ctx.fileNameBuffer);

        logStdout.init(params.logLevel);
        ctx.log = &logStdout;

        ctx.parameters = params;


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
                PollData *pollData = static_cast<PollData*>(events[i].data.ptr);
                ExecutorData &execData = execDatas[pollData->executorDataIndex];

				ProcessResult result = execData.pExecutor->process(execData, pollData->fd, events[i].events);

				if(result == ProcessResult::removeExecutor)
				{
					removeExecutorData(&execData);
				}
				else if(result == ProcessResult::shutdown)
				{
					destroy();
					return -1;
				}
            }
        }

        return 0;
    }

	int enqueueFd(int fd)
	{
		{
			std::lock_guard<std::mutex> lock(newFdsMutex);
			newFds.push(fd);
		}

		eventfd_t value = 1;
		eventfd_write(wakeupFd, value);

		return 0;
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

	std::queue<int> newFds;
	std::mutex newFdsMutex;
};

#endif
