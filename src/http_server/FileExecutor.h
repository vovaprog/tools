#ifndef FILE_EXECUTOR_H
#define FILE_EXECUTOR_H

#include <sys/epoll.h>
#include <errno.h>

#include <ExecutorData.h>
#include <ProcessResult.h>
#include <FileUtils.h>

class FileExecutor: public Executor
{
public:

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
            perror("open failed");
            return -1;
        }

        if(createResponse(data) != 0)
        {
            return -1;
        }

        data.state = ExecutorData::State::sendResponse;

        return 0;
    }

    int process(ExecutorData &data, int fd, int events, ProcessResult &result) override
    {
        if(data.state == ExecutorData::State::sendResponse && fd == data.fd0 && (events & EPOLLOUT))
        {
            return process_sendResponseSendData(data, result);
        }
        if(data.state == ExecutorData::State::sendFile && fd == data.fd0 && (events & EPOLLOUT))
        {
            return process_sendFile(data, result);
        }

        data.ctx->log->warning("invalid process call\n");
        result.action = ProcessResult::Action::none;
        return 0;
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

    int process_sendResponseSendData(ExecutorData &data, ProcessResult &result)
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
                    result.closeResult();
                    return -1;
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

            result.action = ProcessResult::Action::none;
            return 0;
        }
        else
        {
            result.closeResult();
            return -1;
        }
    }

    int process_sendFile(ExecutorData &data, ProcessResult &result)
    {
        int bytesWritten = sendfile(data.fd0, data.fd1, &data.filePosition, data.bytesToSend);
        if(bytesWritten <= 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                result.action = ProcessResult::Action::none;
                return 0;
            }
            else
            {
                perror("sendfile failed");
                result.closeResult();
                return -1;
            }

        }

        data.bytesToSend -= bytesWritten;

        if(data.bytesToSend == 0)
        {
            result.closeResult();
            return 0;
        }

        result.action = ProcessResult::Action::none;
        return 0;
    }
};

#endif
