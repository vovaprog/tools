#ifndef POLL_LOOP_H
#define POLL_LOOP_H

#include <vector>
#include <stack>
#include <unordered_map>
#include <signal.h>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>

#include <RequestExecutor.h>
#include <FileExecutor.h>
#include <UwsgiExecutor.h>
#include <ServerExecutor.h>
#include <ProcessResult.h>
#include <ServerParameters.h>
#include <LogStdout.h>
#include <ExecutorData.h>
#include <PollData.h>
#include <PollLoopBase.h>
#include <ServerBase.h>
#include <NewFdExecutor.h>
#include <ServerBase.h>


class PollLoop: public PollLoopBase
{
public:
	PollLoop(): newFdsQueue(MAX_NEW_FDS) { }

	~PollLoop()
    {
        destroy();
    }

	int init(ServerBase *srv, ServerParameters *params)
	{
		this->srv = srv;
		this->parameters = params;
		log = srv->log;

		strcpy(fileNameBuffer, params->rootFolder);
		strcat(fileNameBuffer, "/");
		rootFolderLength = strlen(fileNameBuffer);


		signal(SIGPIPE, SIG_IGN);


		if(initDataStructs() != 0)
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


		if(createEventFd() != 0)
		{
			destroy();
			return -1;
		}

		serverExecutor.init(this);
		requestExecutor.init(this);
		fileExecutor.init(this);
		uwsgiExecutor.init(this);
		newFdExecutor.init(this);

		numOfPollFds.store(0);

		return 0;
	}


	int run()
	{
		runFlag.store(true);

		while(epollFd > 0 && runFlag.load())
		{
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
			if(!runFlag.load())
			{
				break;
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

		destroy();
		return 0;
	}

	void stop()
	{
		runFlag.store(false);
		eventfd_write(eventFd, 1);
	}

	int enqueueClientFd(int fd)
	{
		{
			std::lock_guard<std::mutex> lock(newFdsMutex);

			if(!newFdsQueue.push(fd))
			{
				return -1;
			}
		}

		eventfd_write(eventFd, 1);

		return 0;
	}

	int checkNewFd() override
	{
		int fd;
		while(newFdsQueue.pop(fd))
		{
			if(createRequestExecutorInternal(fd) != 0)
			{
				close(fd);
			}
		}
		return 0;
	}

	int createRequestExecutor(int fd) override
	{
		if(parameters->threadCount == 1)
		{
			return createRequestExecutorInternal(fd);
		}
		else
		{
			return srv->createRequestExecutor(fd);
		}
	}

	int numberOfPollFds()
	{
		return numOfPollFds.load();
	}

	int addPollFd(ExecutorData &data, int fd, int events) override
	{
		if(emptyPollDatas.size() == 0)
		{
			log->error("too many connections\n");
			return -1;
		}

		if(fd != data.fd0 && fd != data.fd1)
		{
			log->error("invalid argument\n");
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
		++numOfPollFds;

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
			log->error("epoll_ctl failed: %s\n", strerror(errno));
			return -1;
		}
		if(fd == data.fd0)
		{
			emptyPollDatas.push(data.pollIndexFd0);
			--numOfPollFds;
		}
		else if(fd == data.fd1)
		{
			emptyPollDatas.push(data.pollIndexFd1);
			--numOfPollFds;
		}
		return 0;
	}

	int listenPort(int port, ExecutorType execType)
	{
		ExecutorData *pExecData = createExecutorData();

		pExecData->pExecutor = getExecutor(execType);
		pExecData->port = port;

		if(pExecData->pExecutor->up(*pExecData) != 0)
		{
			removeExecutorData(pExecData);
			return -1;
		}

		return 0;
	}

protected:

	int createEventFd()
	{
		eventFd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE); //check flags !!!

		if(eventFd < 0)
		{
			log->error("eventfd failed: %s\n", strerror(errno));
			destroy();
			return -1;
		}

		ExecutorData *execData = createExecutorData();
		execData->fd0 = eventFd;
		execData->pExecutor = &newFdExecutor;

		if(addPollFd(*execData, eventFd, EPOLLIN) != 0)
		{
			removeExecutorData(execData);
			return -1;
		}

		return 0;
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
		if(eventFd > 0)
		{
			close(eventFd);
			eventFd = -1;
		}
    }

	int initDataStructs()
    {
		MAX_EXECUTORS = parameters->maxClients;
		MAX_EVENTS = MAX_EXECUTORS * 2;

		execDatas = new ExecutorData[MAX_EXECUTORS];
        pollDatas = new PollData[MAX_EVENTS];
        events = new epoll_event[MAX_EVENTS];

		for(int i = MAX_EXECUTORS - 1; i >= 0; --i)
        {
            emptyExecDatas.push(i);
        }
        for(int i = MAX_EVENTS - 1; i >= 0; --i)
        {
            emptyPollDatas.push(i);
        }
		for(int i = 0; i < MAX_EXECUTORS; ++i)
        {
			execDatas[i].index = i;
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
			log->error("too many connections\n");
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
			--numOfPollFds;
		}
		if(execData->pollIndexFd1 >= 0)
		{
			emptyPollDatas.push(execData->pollIndexFd1);
			--numOfPollFds;
		}

		execData->down();

		emptyExecDatas.push(execData->index);
	}


	int createRequestExecutorInternal(int fd)
	{
		ExecutorData *pExecData = createExecutorData();

		pExecData->pExecutor = &requestExecutor;
		pExecData->fd0 = fd;

		if(pExecData->pExecutor->up(*pExecData) != 0)
		{
			removeExecutorData(pExecData);
			return -1;
		}

		return 0;
	}


    ServerExecutor serverExecutor;
    RequestExecutor requestExecutor;
    FileExecutor fileExecutor;
    UwsgiExecutor uwsgiExecutor;
	NewFdExecutor newFdExecutor;


    ExecutorData *execDatas = nullptr;
    PollData *pollDatas = nullptr;
    epoll_event *events = nullptr;

    std::stack<int, std::vector<int>> emptyExecDatas;
    std::stack<int, std::vector<int>> emptyPollDatas;

	std::atomic_int numOfPollFds;


	int MAX_EXECUTORS = 0, MAX_EVENTS = 0;

	ServerBase *srv = nullptr;

	std::atomic_bool runFlag;

	static const int MAX_NEW_FDS = 1000;
	boost::lockfree::spsc_queue<int> newFdsQueue;
	std::mutex newFdsMutex;

	int epollFd = -1;
	int eventFd = -1;
};

#endif
