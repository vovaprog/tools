#ifndef UWSGI_EXECUTOR_H
#define UWSGI_EXECUTOR_H

#include <sys/epoll.h>
#include <errno.h>

#include <ExecutorData.h>
#include <ProcessResult.h>
#include <FileUtils.h>
#include <NetworkUtils.h>
#include <ServerBase.h>

class UwsgiExecutor: public Executor
{
public:

	int init(ServerBase *srv)
	{
		this->srv = srv;
		return 0;
	}

    int up(ExecutorData &data) override
    {
        data.fd1 = socketConnect("127.0.0.1", 7070);

        if(data.fd1 < 0)
        {
            data.ctx->log->error("socketConnect failed\n");
            return -1;
        }

        if(setNonBlock(data.fd1) != 0)
        {
            data.ctx->log->error("setNonBlock failed: %s\n", strerror(errno));
            close(data.fd1);
            data.fd1 = -1;
            return -1;
        }

		if(srv->addPollFd(data, data.fd1, EPOLLOUT) != 0)
		{
			return -1;
		}

		if(srv->removePollFd(data, data.fd0) != 0)
		{
			return -1;
		}

        data.state = ExecutorData::State::forwardRequest;

        return 0;
    }

	ProcessResult process(ExecutorData &data, int fd, int events) override
    {
        if(data.state == ExecutorData::State::forwardRequest && fd == data.fd1 && (events & EPOLLOUT))
        {
			return process_forwardRequest(data);
        }
        if(data.state == ExecutorData::State::forwardResponse && fd == data.fd1 && (events & EPOLLIN))
        {
			return process_forwardResponseRead(data);
        }
        if(data.state == ExecutorData::State::forwardResponse && fd == data.fd0 && (events & EPOLLOUT))
        {
			return process_forwardResponseWrite(data);
        }
        if(data.state == ExecutorData::State::forwardResponseOnlyWrite && fd == data.fd0 && (events & EPOLLOUT))
        {
			return process_forwardResponseOnlyWrite(data);
        }

        data.ctx->log->warning("invalid process call\n");
		return ProcessResult::ok;
    }


protected:

	ProcessResult process_forwardRequest(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startRead(p, size))
        {
            char temp[10000];
            strncpy(temp, (char*)p, 400);
            printf("<%s>\n", temp);
            fflush(stdout);


            int bytesWritten = write(data.fd1, p, size);

            if(bytesWritten <= 0)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
					return ProcessResult::ok;
                }
                else
                {
                    data.ctx->log->error("sendfile failed: %s\n", strerror(errno));
					return ProcessResult::removeExecutor;
                }
            }

            data.buffer.endRead(bytesWritten);

            if(bytesWritten == size)
            {
                data.buffer.clear();

                data.state = ExecutorData::State::forwardResponse;

				if(srv->addPollFd(data, data.fd0, EPOLLOUT)!=0)
				{
					return ProcessResult::removeExecutor;
				}
				if(srv->editPollFd(data, data.fd1, EPOLLIN)!=0)
				{
					return ProcessResult::removeExecutor;
				}
				return ProcessResult::ok;
            }

			return ProcessResult::ok;
        }
        else
        {
            data.ctx->log->error("buffer.startRead failed\n");
			return ProcessResult::removeExecutor;
        }
    }

	ProcessResult process_forwardResponseRead(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startWrite(p, size))
        {
            int bytesRead = read(data.fd1, p, size);

            if(bytesRead <= 0)
            {
                if(bytesRead == 0)
                {
                    perror("read failed");
                    close(data.fd1);
                    data.fd1 = -1;
					data.state = ExecutorData::State::forwardResponseOnlyWrite;
					return ProcessResult::ok;
                }
                else if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					return ProcessResult::ok;
                }
                else
                {
                    data.ctx->log->error("read failed: %s\n", strerror(errno));
					return ProcessResult::removeExecutor;
                }
            }
            else
            {
                data.buffer.endWrite(bytesRead);
				return ProcessResult::ok;
            }
        }
        else
        {
			return ProcessResult::ok;
        }
    }

	ProcessResult process_forwardResponseWrite(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startRead(p, size))
        {
            int bytesWritten = write(data.fd0, p, size);

            if(bytesWritten <= 0)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					return ProcessResult::ok;
                }
                else
                {
                    data.ctx->log->error("write failed: %s\n", strerror(errno));
					return ProcessResult::removeExecutor;
                }
            }

            data.buffer.endRead(bytesWritten);
        }

		return ProcessResult::ok;
    }

	ProcessResult process_forwardResponseOnlyWrite(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startRead(p, size))
        {
            int bytesWritten = write(data.fd0, p, size);

            if(bytesWritten <= 0)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
					return ProcessResult::ok;
                }
                else
                {
                    data.ctx->log->error("write failed: %s\n", strerror(errno));
					return ProcessResult::removeExecutor;
                }
            }

            data.buffer.endRead(bytesWritten);

            if(bytesWritten == size && !data.buffer.readAvailable())
            {
				return ProcessResult::removeExecutor;
            }
            else
            {
				return ProcessResult::ok;
            }
        }
        else
        {
			return ProcessResult::removeExecutor;
        }
    }

	ServerBase *srv = nullptr;
};

#endif
