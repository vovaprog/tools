#ifndef PROCESS_RESULT_H
#define PROCESS_RESULT_H

#include <PollData.h>

/*class Executor;

struct ProcessResult
{
    enum class Action { createExecutor, removeExecutor, none, shutdown, setFileExecutor, setUwsgiExecutor, editPoll };

    Action action = Action::none;

    PollData *pollData = nullptr;

    int editFd0Events = 0;
    int editFd1Events = 0;

    int addFd = -1;
    int addFdEvents = 0;

    void reset()
    {
        action = Action::none;
        pollData = nullptr;
        addFd = -1;
        addFdEvents = 0;
        editFd0Events = 0;
        editFd1Events = 0;
    }

    void closeResult()
    {
        action = ProcessResult::Action::removeExecutor;
    }
};*/

enum class ProcessResult { ok, removeExecutor, shutdown };


#endif

