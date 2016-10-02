#ifndef SSL_UWSGI_EXECUTOR_H
#define SSL_UWSGI_EXECUTOR_H

#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>
#include <UwsgiExecutor.h>

class SslUwsgiExecutor: public UwsgiExecutor
{

protected:

    ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count) override
    {
        return SSL_write(data.ssl, buf, count);
    }
};



#endif

