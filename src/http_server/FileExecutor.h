#ifndef FILE_EXECUTOR_H
#define FILE_EXECUTOR_H

#include <sys/epoll.h>
#include <errno.h>

#include <ExecutorData.h>
#include <ProcessResult.h>
#include <FileUtils.h>
#include <ServerBase.h>

class FileExecutor: public Executor
{
public:

	int init(ServerBase *srv)
	{
		this->srv = srv;
		return 0;
	}

    int up(ExecutorData &data) override
    {
        data.bytesToSend = fileSize(data.ctx->fileNameBuffer);

        if(data.bytesToSend < 0)
        {
            data.ctx->log->info("fileSize failed: %s\n", strerror(errno));
            return -1;
        }

        data.fd1 = open(data.ctx->fileNameBuffer, O_NONBLOCK | O_RDONLY);

        if(data.fd1 < 0)
        {
			data.ctx->log->error("open failed: %s\n", strerror(errno));
            return -1;
        }

        if(createResponse(data) != 0)
        {
            return -1;
        }

		if(srv->editPollFd(data, data.fd0, EPOLLOUT) != 0)
		{
			return -1;
		}

        data.state = ExecutorData::State::sendResponse;

        return 0;
    }

	ProcessResult process(ExecutorData &data, int fd, int events) override
    {
        if(data.state == ExecutorData::State::sendResponse && fd == data.fd0 && (events & EPOLLOUT))
        {
			return process_sendResponseSendData(data);
        }
        if(data.state == ExecutorData::State::sendFile && fd == data.fd0 && (events & EPOLLOUT))
        {
			return process_sendFile(data);
        }

        data.ctx->log->warning("invalid process call\n");
		return ProcessResult::ok;
    }


protected:

    int createResponse(ExecutorData &data)
    {
        data.buffer.clear();

        void *p;
        int size;

        if(data.buffer.startWrite(p, size))
        {
            sprintf((char*)p, "HTTP/1.1 200 Ok\r\nContent-Length: %lld\r\nConnection: close\r\n\r\n", data.bytesToSend);
            size = strlen((char*)p);
            data.buffer.endWrite(size);
            return 0;
        }
        else
        {
            data.ctx->log->warning("buffer.startWrite failed\n");
            return -1;
        }
    }

	ProcessResult process_sendResponseSendData(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startRead(p, size))
        {
            int bytesWritten = write(data.fd0, p, size);

            if(bytesWritten <= 0)
            {
                if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
                {
					return ProcessResult::removeExecutor;
                }
            }
            else
            {
                data.buffer.endRead(bytesWritten);

                if(bytesWritten == size)
                {
                    data.state = ExecutorData::State::sendFile;
                }
            }

			return ProcessResult::ok;
        }
        else
        {
			return ProcessResult::removeExecutor;
        }
    }

	ProcessResult process_sendFile(ExecutorData &data)
    {
        int bytesWritten = sendfile(data.fd0, data.fd1, &data.filePosition, data.bytesToSend);
        if(bytesWritten <= 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return ProcessResult::ok;
            }
            else
            {
                perror("sendfile failed");
				return ProcessResult::removeExecutor;
            }

        }

        data.bytesToSend -= bytesWritten;

        if(data.bytesToSend == 0)
        {
			return ProcessResult::removeExecutor;
        }

		return ProcessResult::ok;
    }

	ServerBase *srv = nullptr;
};

#endif
