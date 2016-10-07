#ifndef FILE_EXECUTOR_H
#define FILE_EXECUTOR_H

#include <sys/epoll.h>
#include <errno.h>

#include <ExecutorData.h>
#include <ProcessResult.h>
#include <FileUtils.h>
#include <PollLoopBase.h>

class FileExecutor: public Executor
{
public:

    int init(PollLoopBase *loop) override
    {
        this->loop = loop;
        this->log = loop->log;
        return 0;
    }

    int up(ExecutorData &data) override
    {
        data.removeOnTimeout = true;

        data.fd1 = open(loop->fileNameBuffer, O_NONBLOCK | O_RDONLY);

        if(data.fd1 < 0)
        {
            log->error("open failed: %s\n", strerror(errno));
            return -1;
        }

        data.bytesToSend = fileSize(data.fd1);

        if(data.bytesToSend < 0)
        {
            log->error("fileSize failed: %s\n", strerror(errno));
            return -1;
        }

        if(createResponse(data) != 0)
        {
            return -1;
        }

        if(loop->editPollFd(data, data.fd0, EPOLLOUT) != 0)
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

        log->warning("invalid process call\n");
		return ProcessResult::removeExecutor;
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
            log->warning("buffer.startWrite failed\n");
            return -1;
        }
    }

    ProcessResult process_sendResponseSendData(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startRead(p, size))
        {
            int bytesWritten = writeFd0(data, p, size);

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
                    data.buffer.clear();
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

    virtual ProcessResult process_sendFile(ExecutorData &data)
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
                log->error("sendfile failed: %s\n", strerror(errno));
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
};

#endif
