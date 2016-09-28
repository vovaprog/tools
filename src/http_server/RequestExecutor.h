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

#include <Executor.h>
#include <TransferRingBuffer.h>
#include <FileUtils.h>
#include <ServerParameters.h>
#include <ServerContext.h>
#include <NetworkUtils.h>
#include <ExecutorData.h>
#include <ServerBase.h>

class RequestExecutor: public Executor
{
public:
	int init(ServerBase *srv)
	{
		this->srv = srv;
		return 0;
	}

    int up(ExecutorData &data) override
    {
        data.buffer.init(ExecutorData::REQUEST_BUFFER_SIZE);
        return 0;
    }

	ProcessResult process(ExecutorData &data, int fd, int events) override
    {
        if(data.state == ExecutorData::State::readRequest && fd == data.fd0 && (events & EPOLLIN))
        {
			return process_readRequestReadSocket(data);
        }

		data.ctx->log->warning("invalid process call\n");
		return ProcessResult::ok;
    }


protected:

    int readRequest(ExecutorData &data)
    {
        void *p;
        int size;

        if(data.buffer.startWrite(p, size))
        {
            int rd = read(data.fd0, p, size);

            if(rd <= 0)
            {
                if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
                {
                    if(rd == 0 && errno == 0)
                    {
                        data.ctx->log->debug("client disconnected\n");
                    }
                    else
                    {
                        data.ctx->log->info("read failed: %s\n", strerror(errno));
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
            data.ctx->log->warning("requestBuffer.startWrite failed");
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
                    urlBuffer[i] == '=' || urlBuffer[i] == '?' || urlBuffer[i] == '-'))
            {
                return -1;
            }
            if(urlBuffer[i] == '.' && urlBuffer[i] == prevChar)
            {
                return -1;
            }
            prevChar = urlBuffer[i];
        }
        if(data.ctx->rootFolderLength + i > ServerContext::MAX_FILE_NAME)
        {
            return -1;
        }
        return 0;
    }

    bool isUrlPrefix(char *url, char *prefix)
    {
        int i;
        for(i = 0; url[i] != 0 && prefix[i] != 0 && url[i] == prefix[i]; ++i);

        if((prefix[i] == 0 || prefix[i] == '/') && (url[i] == 0 || url[i] == '/' || url[i] == '?'))
        {
            return true;
        }

        return false;
    }

    enum class ParseRequestResult { again, file, uwsgi, invalid };

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

                    if(sscanf(cdata, "GET %s HTTP", urlBuffer) == 1)
                    {
                        if(checkUrl(data, urlBuffer) != 0)
                        {
                            data.ctx->log->info("invalid url\n");
                            return ParseRequestResult::invalid;
                        }

                        data.ctx->log->debug("url: %s\n", urlBuffer);

                        data.ctx->fileNameBuffer[data.ctx->rootFolderLength] = 0;

                        if(strcmp(urlBuffer, "/") == 0)
                        {
                            strcat(data.ctx->fileNameBuffer, "index.html");
                        }
                        else
                        {
                            strcat(data.ctx->fileNameBuffer, urlBuffer);
                        }

                        for(int i = 0; i < ServerParameters::MAX_APPLICATIONS; ++i)
                        {
                            if(data.ctx->parameters.wsgiApplications[i][0] == 0)
                            {
                                break;
                            }

                            if(isUrlPrefix(urlBuffer, data.ctx->parameters.wsgiApplications[i]))
                            {
                                return ParseRequestResult::uwsgi;
                            }
                        }

                        return ParseRequestResult::file;
                    }
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

	ProcessResult process_readRequestReadSocket(ExecutorData &data)
    {
        if(readRequest(data) != 0)
        {
			return ProcessResult::removeExecutor;
        }

        ParseRequestResult parseResult = parseRequest(data);

        if(parseResult == ParseRequestResult::file)
        {
			return setExecutor(data, srv->getExecutor(ExecutorType::file));
        }
        else if(parseResult == ParseRequestResult::uwsgi)
        {
			return setExecutor(data, srv->getExecutor(ExecutorType::uwsgi));
        }
        else if(parseResult == ParseRequestResult::invalid)
        {
			return ProcessResult::removeExecutor;
        }

		return ProcessResult::ok;
    }

	ServerBase *srv = nullptr;
};

#endif
