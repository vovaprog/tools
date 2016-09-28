#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <ProcessResult.h>
#include <ExecutorData.h>

class Executor
{
public:

    virtual int up(ExecutorData &data) = 0;

	virtual ProcessResult process(ExecutorData &data, int fd, int events) = 0;

};

#endif
