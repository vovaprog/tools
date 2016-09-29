#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <ServerParameters.h>
#include <PollLoop.h>

class Server{


	int run(ServerParameters &params)
	{
		loops = new PollLoop[params.threadCount];

		for(int i=0;i<params.threadCount;++i)
		{
			loops[i].init(this, params);
		}

		//=================================================

		int i = 0;
		for(int port : params.httpPorts)
		{
			loops[i % params.threadCount].listenHttp(port);
			++i;
		}
		for(int port : params.httpsPorts)
		{
			loops[i % params.threadCount].listenHttps(port);
			++i;
		}

		//=================================================

		if(params.threadCount > 1)
		{
			threads = new std::thread[params.threadCount - 1];

			for(int i=0;i<params.threadCount - 1;++i)
			{
				threads[i] = std::thread(PollLoop::run, &loops[i], &params);
			}
		}

		//=================================================

		loops[params.threadCount - 1].run(params);

		//=================================================

		for(int i=0;i<params.threadCount - 1;++i)
		{
			loops[i].stop();
		}

		for(int i=0;i<params.threadCount - 1;++i)
		{
			threads[i].join();
		}

		delete[] loops;
		loops = nullptr;

		delete[] threads;
		threads = nullptr;
	}

	int createRequestExecutor(int fd)
	{
		int minFds = MAX_INT;
		int minIndex = 0;

		for(int i=0;i<params;++i)
		{
			int numberOfFds = loops[i].numberOfFds();
			if(numberOfFds < minFds)
			{
				minFds = numberOfFds;
				minIndex = i;
			}
		}

		loops[i].enqueueFd(fd);

		return 0;
	}

	ServerParameters params;
	PollLoop *loops;
	std::thread *threads;
};




#endif

