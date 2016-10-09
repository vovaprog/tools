#ifndef NEW_FD_EXECUTOR_H
#define NEW_FD_EXECUTOR_H

#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>

class NewFdExecutor: public Executor
{
public:
    int init(PollLoopBase *loop) override
    {
        this->loop = loop;
        log = loop->log;
        return 0;
    }


    int up(ExecutorData &data) override
    {
        data.removeOnTimeout = false;

        return 0;
    }


    ProcessResult process(ExecutorData &data, int fd, int events) override
    {
        eventfd_t val;
        if(eventfd_read(fd, &val) != 0)
        {
            log->error("eventfd_failed\n");
            return ProcessResult::shutdown;
        }

        loop->checkNewFd();
        return ProcessResult::ok;
    }
};



#endif

