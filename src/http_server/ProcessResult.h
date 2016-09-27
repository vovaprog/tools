#ifndef PROCESS_RESULT_H
#define PROCESS_RESULT_H

#include <PollData.h>

class Executor;

struct ProcessResult {
	enum class Action { createExecutor, removeExecutor, editPoll, none, shutdown, setFileExecutor, setUwsgiExecutor };

	Action action = Action::none;

	PollData *pollData = nullptr;

	int addFd = -1;
	int addFdEvents = 0;

	int editFd = -1;
	int editFdEvents = 0;

	void reset()
	{
		action = Action::none;
		pollData = nullptr;
		addFd = -1;
		addFdEvents = 0;
		editFd = -1;
		editFdEvents = 0;
	}

	void closeResult()
	{
		action = ProcessResult::Action::removeExecutor;
	}
};

#endif

