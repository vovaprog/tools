#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <ProcessResult.h>
#include <ExecutorData.h>

class Executor {
public:	

	virtual int up(ExecutorData &data) = 0;
	virtual int process(ExecutorData &data, int fd, int events, ProcessResult &result) = 0;

};

#endif
