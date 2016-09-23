#ifndef PROCESS_RESULT_H
#define PROCESS_RESULT_H

class Executor;
class DescriptorExecutor;

struct ProcessResult {
	enum class Action { createExecutor, removeExecutor, editPoll, None };

	Action action = Action::None;

	Executor *pExecutor = nullptr;
	DescriptorExecutor *pDe = nullptr;

	int addFd = -1;
	int addFdEvents = 0;

	int editFd = -1;
	int editFdEvents = 0;

	void reset()
	{
		action = Action::None;
		pExecutor = nullptr;
		pDe = nullptr;
		addFd = -1;
		addFdEvents = 0;
		editFd = -1;
		editFdEvents = 0;
	}
};

#endif

