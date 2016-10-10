#ifndef UWSGI_EXECUTOR_H
#define UWSGI_EXECUTOR_H

#include <Executor.h>

class UwsgiExecutor: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

protected:

    ProcessResult process_forwardRequest(ExecutorData &data);

    ProcessResult process_forwardResponseRead(ExecutorData &data);

    ProcessResult process_forwardResponseWrite(ExecutorData &data);

    ProcessResult process_forwardResponseOnlyWrite(ExecutorData &data);
};

#endif
