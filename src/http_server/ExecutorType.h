#ifndef EXECUTOR_TYPE_H
#define EXECUTOR_TYPE_H

enum class ExecutorType
{
    server, serverSsl, request, requestSsl, file, sslFile, uwsgi
};

#endif
