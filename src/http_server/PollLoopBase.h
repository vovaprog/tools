#ifndef SERVER_BASE_H
#define SERVER_BASE_H

#include <Executor.h>
#include <ExecutorData.h>

enum class ExecutorType { server, request, file, uwsgi };

class PollLoopBase {
public:
	virtual ExecutorData* createExecutorData() = 0;
	virtual void removeExecutorData(ExecutorData* data) = 0;

	virtual Executor* getExecutor(ExecutorType execType) = 0;

	virtual int addPollFd(ExecutorData &data, int fd, int events) = 0;
	virtual int editPollFd(ExecutorData &data, int fd, int events) = 0;
	virtual int removePollFd(ExecutorData &data, int fd) = 0;
};

#endif
