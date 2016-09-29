#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <ServerParameters.h>
#include <PollLoop.h>
#include <ServerBase.h>
#include <ExecutorType.h>
#include <climits>

class Server: public ServerBase{
public:

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
			loops[i % parameters.threadCount].listenPort(port, ExecutorType::server);
			++i;
		}
		for(int port : parameters.httpsPorts)
		{
			loops[i % parameters.threadCount].listenPort(port, ExecutorType::serverSsl);
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

		for(int i=0;i<parameters.threadCount - 1;++i)
		{
			loops[i].stop();
		}

		for(int i=0;i<parameters.threadCount - 1;++i)
		{
			threads[i].join();
		}

		delete[] loops;
		loops = nullptr;

		delete[] threads;
		threads = nullptr;

		return 0;
	}

	void destroy()
	{

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

		return loops[minIndex].enqueueClientFd(fd);
	}

	ServerParameters parameters;

	PollLoop *loops;
	std::thread *threads;
};




#endif

