#ifndef EXECUTOR_H
#define EXECUTOR_H

class Executor {
public:
	virtual ProcessResult process(int fd, int events) = 0;
};

#endif
