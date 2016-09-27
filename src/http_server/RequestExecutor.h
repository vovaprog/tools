#ifndef CLIENT_EXECUTOR_H
#define CLIENT_EXECUTOR_H

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

	int process(ExecutorData &data, int fd, int events, ProcessResult &result) override
	{
		if(data.state == ExecutorData::State::readRequest && fd == clientSocketFd && (events & EPOLLIN))
		{
			return process_readRequestReadSocket(data, result);
		}

		ctx->log->warning("invalid process call\n");
		result.action = ProcessResult::Action::none;
		return 0;
	}


protected:

    int readRequest()
    {
		void *data;
		int size;

        if(buffer.startWrite(data, size))
        {
			int rd = read(clientSocketFd, data, size);
    
            if(rd <= 0)
            {
                if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
                {
                    if(rd == 0 && errno == 0)
                    {
						ctx->log->debug("client disconnected\n");
                    }
                    else
                    {
						ctx->log->info("read failed: %s\n", strerror(errno));
					}
					return -1;
                }
            }
            else
            {            
                buffer.endWrite(rd);
            }
		}
		else
		{
			ctx->log->warning("requestBuffer.startWrite failed");
			return -1;
		}

		return 0;
    }
    
    void closeResult(ProcessResult &result)
    {
        result.action = ProcessResult::Action::removeExecutor;
		state = State::invalid;
    }
    
	int checkUrl(char *urlBuffer)
    {
        char prevChar = 0;
        int i;
        for(i=0;i < REQUEST_BUFFER_SIZE && urlBuffer[i]!=0 ;++i)
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
		if(ctx->rootFolderLength + i > ServerContext::MAX_FILE_NAME)
        {
            return -1;
        }
        return 0;
    }

	enum class ParseRequestResult { again, file, uwsgi, invalid };

	ParseRequestResult parseRequest()
	{
        void *data;
        int size;

        if(buffer.startRead(data, size))
        {
			if(size > 1)
			{
				char *cdata = (char*)data;

				char *endOfHeaders = strstr(cdata, "\r\n\r\n");

				if(endOfHeaders != nullptr)
				{
					char saveChar;

					saveChar = cdata[size - 1];
					cdata[size - 1] = 0;

					char urlBuffer[REQUEST_BUFFER_SIZE];

					if(sscanf(cdata, "GET %s HTTP", urlBuffer) == 1)
					{
						if(checkUrl(urlBuffer) != 0)
						{
							ctx->log->info("invalid url\n");
							return ParseRequestResult::invalid;
						}

						ctx->log->debug("url: %s\n", urlBuffer);

						ctx->fileNameBuffer[ctx->rootFolderLength] = 0;

						if(strcmp(urlBuffer, "/") == 0)
						{
							strcat(ctx->fileNameBuffer, "index.html");
						}
						else
						{
							strcat(ctx->fileNameBuffer, urlBuffer);
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

    int createResponse()
    {
        buffer.clear();

        void *data;
        int size;

        if(buffer.startWrite(data, size))
        {
			sprintf((char*)data,"HTTP/1.1 200 Ok\r\nContent-Length: %lld\r\nConnection: close\r\n\r\n", bytesToSend);
            size = strlen((char*)data);
            buffer.endWrite(size);
            return 0;
        }
        else
        {
			ctx->log->warning("buffer.startWrite failed\n");
            return -1;
        }
    }


	int process_readRequestReadSocket(ExecutorData &data, ProcessResult &result)
    {        
        if(readRequest() != 0)
        {
            closeResult(result);
            return -1;
        }
        
		ParseRequestResult parseResult = parseRequest();

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
			closeResult(result);
			return -1;
		}

		result.action = ProcessResult::Action::none;
		return 0;
	}
};

#endif
