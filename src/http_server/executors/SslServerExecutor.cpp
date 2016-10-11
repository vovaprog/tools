#include <SslServerExecutor.h>
#include <PollLoopBase.h>
#include <NetworkUtils.h>

#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>


int SslServerExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    log = loop->log;
    return 0;
}


int SslServerExecutor::up(ExecutorData &data)
{
    data.removeOnTimeout = false;

    data.fd0 = socketListen(data.port);
    if(data.fd0 < 0)
    {
        return -1;
    }

    if(loop->addPollFd(data, data.fd0, EPOLLIN) != 0)
    {
        return -1;
    }

    return 0;
}


ProcessResult SslServerExecutor::process(ExecutorData &data, int fd, int events)
{
    if(fd != data.fd0)
    {
        log->error("invalid file\n");
        return ProcessResult::ok;
    }

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    int clientSockFd = accept4(data.fd0, (struct sockaddr *)&address, (socklen_t*)&addrlen, SOCK_NONBLOCK);

    if(clientSockFd == -1)
    {
        log->error("accept failed: %s", strerror(errno));
        return ProcessResult::shutdown;
    }


    loop->createRequestExecutor(clientSockFd, ExecutorType::requestSsl);

    return ProcessResult::ok;
}


