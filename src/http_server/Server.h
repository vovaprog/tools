#ifndef SERVER_H
#define SERVER_H

#include <thread>
#include <climits>

#include <ServerBase.h>
#include <ServerParameters.h>
#include <PollLoop.h>
#include <ExecutorType.h>
#include <Log.h>
#include <LogStdout.h>
#include <LogMmap.h>
#include <SslUtils.h>


class Server: public ServerBase
{
public:

    ~Server()
    {
        stop();
    }

    int start(ServerParameters &parameters)
    {
        this->parameters = parameters;


        if(parameters.logType == Log::Type::stdout)
        {
            log = new LogStdout();
        }
        else
        {
            log = new LogMmap();
        }

        if(log->init(&parameters) != 0)
        {
            return -1;
        }

        if(parameters.httpsPorts.size() > 0)
        {
            if(initSsl(globalSslCtx, log) != 0)
            {
                return -1;
            }
        }


        loops = new PollLoop[parameters.threadCount];

        for(int i = 0; i < parameters.threadCount; ++i)
        {
            if(loops[i].init(this, &parameters) != 0)
            {
                stop();
                return -1;
            }
        }

        //=================================================

        int i = 0;
        for(int port : parameters.httpPorts)
        {
            if(loops[i % parameters.threadCount].listenPort(port, ExecutorType::server) != 0)
            {
                stop();
                return -1;
            }
            ++i;
        }
        for(int port : parameters.httpsPorts)
        {
            if(loops[i % parameters.threadCount].listenPort(port, ExecutorType::serverSsl) != 0)
            {
                stop();
                return -1;
            }
            ++i;
        }

        //=================================================

        threads = new std::thread[parameters.threadCount];

        for(int i = 0; i < parameters.threadCount; ++i)
        {
            threads[i] = std::thread(&PollLoop::run, &loops[i]);
        }

        return 0;
    }

    void stop()
    {
        if(loops != nullptr)
        {
            for(int i = 0; i < parameters.threadCount; ++i)
            {
                loops[i].stop();
            }
        }

        if(threads != nullptr)
        {
            for(int i = 0; i < parameters.threadCount; ++i)
            {
                threads[i].join();
            }
        }

        if(loops != nullptr)
        {
            delete[] loops;
            loops = nullptr;
        }

        if(threads != nullptr)
        {
            delete[] threads;
            threads = nullptr;
        }

        if(globalSslCtx != nullptr)
        {
            destroySsl(globalSslCtx);
            globalSslCtx = nullptr;
        }

        if(log != nullptr)
        {
            delete log;
            log = nullptr;
        }
    }

    int createRequestExecutor(int fd, ExecutorType execType) override
    {
        int minPollFds = INT_MAX;
        int minIndex = 0;

        for(int i = 0; i < parameters.threadCount; ++i)
        {
            int numberOfFds = loops[i].numberOfPollFds();
            if(numberOfFds < minPollFds)
            {
                minPollFds = numberOfFds;
                minIndex = i;
            }
        }

        if(loops[minIndex].enqueueClientFd(fd, execType) != 0)
        {
            log->error("enqueueClientFd failed\n");
            close(fd);
            return -1;
        }

        return 0;
    }

    void logStats()
    {
        int totalNumberOfFds = 0;

        for(int i = 0; i < parameters.threadCount; ++i)
        {
            int numberOfFds = loops[i].numberOfPollFds();
            totalNumberOfFds += numberOfFds;
            log->info("poll files. thread %d: %d\n", i, numberOfFds);
        }

        log->info("poll files. total:    %d\n", totalNumberOfFds);
    }


protected:

    ServerParameters parameters;

    PollLoop *loops = nullptr;
    std::thread *threads = nullptr;
};

#endif

