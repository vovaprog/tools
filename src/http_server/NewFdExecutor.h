#ifndef NEW_FD_EXECUTOR_H
#define NEW_FD_EXECUTOR_H

#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>

class NewFdExecutor: public Executor
{
public:
	int init(PollLoopBase *loop)
	{
		this->loop = loop;
		return 0;
	}


	virtual int up(ExecutorData &data)
	{
		return 0;
	}

	virtual ProcessResult process(ExecutorData &data, int fd, int events)
	{
		loop->checkNewFd();
		return ProcessResult::ok;
	}

	PollLoopBase *loop;
};



#endif

