#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <ServerParameters.h>
#include <PollLoop.h>
#include <ServerBase.h>
#include <ExecutorType.h>
#include <climits>
#include <Log.h>
#include <LogStdout.h>

class Server: public ServerBase{
public:

	~Server()
	{
		destroy();
	}

	int run(ServerParameters &parameters)
	{
		this->parameters = parameters;

		loops = new PollLoop[parameters.threadCount];

		for(int i=0;i<parameters.threadCount;++i)
		{
			if(loops[i].init(this, &parameters) != 0)
			{
				destroy();
				return -1;
			}
		}

		//=================================================

		int i = 0;
		for(int port : parameters.httpPorts)
		{
			if(loops[i % parameters.threadCount].listenPort(port, ExecutorType::server) != 0)
			{
				destroy();
				return -1;
			}
			++i;
		}
		for(int port : parameters.httpsPorts)
		{
			if(loops[i % parameters.threadCount].listenPort(port, ExecutorType::serverSsl) != 0)
			{
				destroy();
				return -1;
			}
			++i;
		}

		//=================================================

		if(parameters.threadCount > 1)
		{
			threads = new std::thread[parameters.threadCount - 1];

			for(int i=0;i<parameters.threadCount - 1;++i)
			{
				threads[i] = std::thread(&PollLoop::run, &loops[i]);
			}
		}

		//=================================================

		loops[parameters.threadCount - 1].run();

		//=================================================

		destroy();

		return 0;
	}

	void destroy()
	{
		if(loops != nullptr)
		{
			for(int i=0;i<parameters.threadCount;++i)
			{
				loops[i].stop();
			}
		}

		if(threads != nullptr)
		{
			for(int i=0;i<parameters.threadCount - 1;++i)
			{
				threads[i].join();
			}
		}

		if(loops != nullptr)
		{
			delete[] loops;
			loops = nullptr;
		}

		if(threads!= nullptr)
		{
			delete[] threads;
			threads = nullptr;
		}
	}

	int createRequestExecutor(int fd)
	{
		int minFds = INT_MAX;
		int minIndex = 0;

		for(int i=0;i<parameters.threadCount;++i)
		{
			int numberOfFds = loops[i].numberOfFds();
			if(numberOfFds < minFds)
			{
				minFds = numberOfFds;
				minIndex = i;
			}
		}

		if(loops[minIndex].enqueueClientFd(fd) != 0)
		{
			log->error("enqueueClientFd failed\n");
			close(fd);
			return -1;
		}

		return 0;
	}

protected:

	ServerParameters parameters;

	LogStdout logStdout;
	Log *log = nullptr;

	PollLoop *loops = nullptr;
	std::thread *threads = nullptr;
};

#endif

