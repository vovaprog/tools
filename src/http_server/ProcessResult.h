#ifndef PROCESS_RESULT_H
#define PROCESS_RESULT_H

class Executor;

struct DescriptorExecutor
{
	int fd = -1;
	//Executor *pExecutor = nullptr;
	int executorIndex = -1;
};

struct ProcessResult {
	enum class Action { createExecutor, removeExecutor, editPoll, none, shutdown };

	Action action = Action::none;

	DescriptorExecutor *pDe = nullptr;

	int addFd = -1;
	int addFdEvents = 0;

	int editFd = -1;
	int editFdEvents = 0;

	void reset()
	{
		action = Action::none;
		pDe = nullptr;
		addFd = -1;
		addFdEvents = 0;
		editFd = -1;
		editFdEvents = 0;
	}
};

#endif

