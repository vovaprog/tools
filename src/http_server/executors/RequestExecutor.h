#ifndef REQUEST_EXECUTOR_H
#define REQUEST_EXECUTOR_H

#include <sys/sendfile.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>

#include <Executor.h>
#include <TransferRingBuffer.h>
#include <FileUtils.h>
#include <ServerParameters.h>
#include <NetworkUtils.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>
#include <percent_decode.h>

class RequestExecutor: public Executor
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
        data.buffer.init(ExecutorData::REQUEST_BUFFER_SIZE);

        if(loop->addPollFd(data, data.fd0, EPOLLIN) != 0)
        {
            return -1;
        }

        data.state = ExecutorData::State::readRequest;

        return 0;
    }

    ProcessResult process(ExecutorData &data, int fd, int events) override
    {
        if(data.state == ExecutorData::State::readRequest && fd == data.fd0 && (events & EPOLLIN))
        {
            return process_readRequest(data);
        }

        log->warning("invalid process call\n");
        return ProcessResult::ok;
    }


protected:

    int readRequest(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startWrite(p, size))
        {
            int rd = readFd0(data, p, size);

            if(rd <= 0)
            {
                if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
                {
                    if(rd == 0 && errno == 0)
                    {
                        log->debug("client disconnected\n");
                    }
                    else
                    {
                        log->info("read failed: %s\n", strerror(errno));
                    }
                    return -1;
                }
            }
            else
            {
                data.buffer.endWrite(rd);
            }
        }
        else
        {
            log->warning("requestBuffer.startWrite failed");
            return -1;
        }

        return 0;
    }

    int checkUrl(ExecutorData &data, char *urlBuffer)
    {
        char prevChar = 0;
        int i;
        for(i = 0; i < ExecutorData::REQUEST_BUFFER_SIZE && urlBuffer[i] != 0 ; ++i)
        {
            if(!((urlBuffer[i] >= 'a' && urlBuffer[i] <= 'z') ||
                    (urlBuffer[i] >= 'A' && urlBuffer[i] <= 'Z') ||
                    (urlBuffer[i] >= '0' && urlBuffer[i] <= '9') ||
                    urlBuffer[i] == '/' || urlBuffer[i] == '.' || urlBuffer[i] == '_' ||
                    urlBuffer[i] == '=' || urlBuffer[i] == '?' || urlBuffer[i] == '-' || urlBuffer[i] == ' '))
            {
                return -1;
            }
            if(urlBuffer[i] == '.' && urlBuffer[i] == prevChar)
            {
                return -1;
            }
            prevChar = urlBuffer[i];
        }
        if(loop->rootFolderLength + i > PollLoopBase::MAX_FILE_NAME)
        {
            return -1;
        }
        return 0;
    }

    bool isUrlPrefix(const char *url, const char *prefix)
    {
        int i;
        for(i = 0; url[i] != 0 && prefix[i] != 0 && url[i] == prefix[i]; ++i);

        if((prefix[i] == 0 || prefix[i] == '/') && (url[i] == 0 || url[i] == '/' || url[i] == '?'))
        {
            return true;
        }

        return false;
    }

    enum class ParseRequestResult
    {
        again, file, uwsgi, invalid
    };

    ParseRequestResult parseRequest(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startRead(p, size))
        {
            if(size > 1)
            {
                char *cdata = (char*)p;

                char *endOfHeaders = strstr(cdata, "\r\n\r\n");

                if(endOfHeaders != nullptr)
                {
                    if(size < ExecutorData::REQUEST_BUFFER_SIZE)
                    {
                        cdata[size] = 0;
                    }
                    else
                    {
                        return ParseRequestResult::invalid;
                    }

                    char urlBuffer[ExecutorData::REQUEST_BUFFER_SIZE];

                    if(sscanf(cdata, "GET %s HTTP", urlBuffer) != 1)
                    {
                        if(sscanf(cdata, "POST %s HTTP", urlBuffer) != 1)
                        {
                            log->info("invalid url\n");
                            return ParseRequestResult::invalid;
                        }
                    }

                    if(percent_decode_in_place(urlBuffer, 1) == nullptr)
                    {
                        return ParseRequestResult::invalid;
                    }

                    if(checkUrl(data, urlBuffer) != 0)
                    {
                        log->info("invalid url\n");
                        return ParseRequestResult::invalid;
                    }

                    log->debug("url: %s\n", urlBuffer);

                    loop->fileNameBuffer[loop->rootFolderLength] = 0;

                    if(strcmp(urlBuffer, "/") == 0)
                    {
                        strcat(loop->fileNameBuffer, "index.html");
                    }
                    else
                    {
                        strcat(loop->fileNameBuffer, urlBuffer);
                    }

                    for(std::string & app : loop->parameters->uwsgiApplications)
                    {
                        if(isUrlPrefix(urlBuffer, app.c_str()))
                        {
                            return ParseRequestResult::uwsgi;
                        }
                    }

                    return ParseRequestResult::file;
                }
            }
        }
        return ParseRequestResult::again;
    }

    ProcessResult setExecutor(ExecutorData &data, Executor *pExecutor)
    {
        data.pExecutor = pExecutor;
        if(pExecutor->up(data) != 0)
        {
            return ProcessResult::removeExecutor;
        }
        else
        {
            return ProcessResult::ok;
        }
    }

    virtual ProcessResult process_readRequest(ExecutorData &data)
    {
        if(readRequest(data) != 0)
        {
            return ProcessResult::removeExecutor;
        }

        ParseRequestResult parseResult = parseRequest(data);

        if(parseResult == ParseRequestResult::file)
        {
            return setExecutor(data, loop->getExecutor(ExecutorType::file));
        }
        else if(parseResult == ParseRequestResult::uwsgi)
        {
            return setExecutor(data, loop->getExecutor(ExecutorType::uwsgi));
        }
        else if(parseResult == ParseRequestResult::invalid)
        {
            return ProcessResult::removeExecutor;
        }

        return ProcessResult::ok;
    }
};

#endif
