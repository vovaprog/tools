#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ProcessResult.h"

class Executor {
public:
	virtual int process(int fd, int events, ProcessResult &result);
};

#endif
