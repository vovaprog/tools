#ifndef NEW_FD_EXECUTOR_H
#define NEW_FD_EXECUTOR_H

#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>

class NewFdExecutor: public Executor
{
public:
    int init(PollLoopBase *loop) override
    {
        this->loop = loop;
        return 0;
    }


    int up(ExecutorData &data) override
    {
        return 0;
    }


    ProcessResult process(ExecutorData &data, int fd, int events) override
    {
        loop->checkNewFd();
        return ProcessResult::ok;
    }

    PollLoopBase *loop;
};



#endif

