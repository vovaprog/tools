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

class RequestExecutor: public Executor {
public:

	int up(ExecutorData &data) override
	{
		data.buffer.init(ExecutorData::REQUEST_BUFFER_SIZE);
		return 0;
	}

	int process(ExecutorData &data, int fd, int events, ProcessResult &result) override
	{
		if(data.state == ExecutorData::State::readRequest && fd == data.fd0 && (events & EPOLLIN))
		{
			return process_readRequestReadSocket(data, result);
		}

		data.ctx->log->warning("invalid process call\n");
		result.action = ProcessResult::Action::none;
		return 0;
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
		for(i=0;i < ExecutorData::REQUEST_BUFFER_SIZE && urlBuffer[i]!=0 ;++i)
        {
            if(!((urlBuffer[i]>='a' && urlBuffer[i]<='z') ||
               (urlBuffer[i]>='A' && urlBuffer[i]<='Z') ||
               (urlBuffer[i]>='0' && urlBuffer[i]<='9') ||
               urlBuffer[i]=='/' || urlBuffer[i]=='.'))
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
					char saveChar;

					saveChar = cdata[size - 1];
					cdata[size - 1] = 0;

					char urlBuffer[ExecutorData::REQUEST_BUFFER_SIZE];

					if(sscanf(cdata, "GET %s HTTP", urlBuffer) == 1)
					{
						if(checkUrl(data,urlBuffer) != 0)
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

						if(strncmp(urlBuffer,"/gallery",strlen("/gallery"))==0)
						{
							cdata[size - 1] = saveChar;
							return ParseRequestResult::uwsgi;
						}

						return ParseRequestResult::file;
					}

					cdata[size - 1] = saveChar;
				}
			}
        }
		return ParseRequestResult::again;
	}


	int process_readRequestReadSocket(ExecutorData &data, ProcessResult &result)
    {        
		if(readRequest(data) != 0)
        {
			result.closeResult();
            return -1;
        }
        
		ParseRequestResult parseResult = parseRequest(data);

		if(parseResult == ParseRequestResult::file)
        {
			result.action = ProcessResult::Action::setFileExecutor;
			return 0;
		}
		else if(parseResult == ParseRequestResult::uwsgi)
		{
			result.action = ProcessResult::Action::setUwsgiExecutor;
			return 0;
		}
		else if(parseResult == ParseRequestResult::invalid)
		{
			result.closeResult();
			return -1;
		}

		result.action = ProcessResult::Action::none;
		return 0;
	}
};

#endif
