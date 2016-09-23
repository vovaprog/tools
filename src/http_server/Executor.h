#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ProcessResult.h"

class Executor {
public:
	static const int MAX_FDS_OF_EXECUTOR = 2;

	virtual int process(int fd, int events, ProcessResult &result) = 0;
	virtual int getFds(int *fds) = 0;
};

#endif
