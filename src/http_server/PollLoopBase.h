#ifndef POLL_LOOP_BASE_H
#define POLL_LOOP_BASE_H

#include <Executor.h>
#include <ExecutorData.h>
#include <Log.h>
#include <ServerParameters.h>
#include <ExecutorType.h>

class PollLoopBase {
public:
	virtual ExecutorData* createExecutorData() = 0;
	virtual void removeExecutorData(ExecutorData* data) = 0;

	virtual Executor* getExecutor(ExecutorType execType) = 0;

	virtual int addPollFd(ExecutorData &data, int fd, int events) = 0;
	virtual int editPollFd(ExecutorData &data, int fd, int events) = 0;
	virtual int removePollFd(ExecutorData &data, int fd) = 0;

	virtual int createRequestExecutor(int fd) = 0;
	virtual int checkNewFd() = 0;

	Log *log;

	static const int MAX_FILE_NAME = 300;
	char fileNameBuffer[MAX_FILE_NAME + 1];
	int rootFolderLength = 0;

	ServerParameters *parameters;
};

#endif
