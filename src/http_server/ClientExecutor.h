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

class ClientExecutor: public Executor {
public:    
    ~ClientExecutor()
    {
        down();
    }

    int init(ServerParameters &params)
    {
        strcpy(fileName, params.rootFolder);
        strcat(fileName, "/");
        rootFolderLength = strlen(fileName);

        buffer.init(REQUEST_BUFFER_SIZE);

        return 0;
    }

    int up(int clientSocketFd)
    {
        down();
        
        this->clientSocketFd = clientSocketFd;        
		bytesToSend = 0;
		filePosition = 0;        
        buffer.clear();

        state = State::readRequest;

        return 0;
    }
    
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
                        printf("client disconnected\n");
                    }
                    else
                    {
                        printf("read failed: %s\n", strerror(errno));
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
			printf("requestBuffer.startWrite failed");
			return -1;
		}

		return 0;
    }
    
    void closeResult(ProcessResult &result)
    {
        result.action = ProcessResult::Action::removeExecutor;
		state = State::invalid;
    }
    
    int checkUrl(char *url)
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
        if(rootFolderLength + i > MAX_FILE_NAME)
        {
            return -1;
        }
        return 0;
    }

	int parseRequest()
	{
        void *data;
        int size;

        if(buffer.startRead(data, size))
        {
            char *cdata = (char*)data;

            char saveChar;
            if(size > 1)
            {
                saveChar = cdata[size - 1];
                cdata[size - 1] = 0;

                if(sscanf(cdata, "GET %s HTTP", urlBuffer) == 1)
                {
                    if(checkUrl(urlBuffer) != 0)
                    {
                        printf("invalid url\n");
                        return -2;
                    }

                    printf("url: %s\n", urlBuffer);

                    fileName[rootFolderLength] = 0;

                    if(strcmp(urlBuffer, "/") == 0)
                    {
                        strcat(fileName, "index.html");
                    }
                    else
                    {
                        strcat(fileName, urlBuffer);
                    }

                    return 0;
                }

                cdata[size - 1] = saveChar;
            }
        }
        return -1;
	}

    int createResponse()
    {
        buffer.clear();

        void *data;
        int size;

        if(buffer.startWrite(data, size))
        {
            sprintf((char*)data,"HTTP/1.1 200 Ok\r\nContent-Length: %lld\r\n\r\n", bytesToSend);
            size = strlen((char*)data);
            buffer.endWrite(size);
            return 0;
        }
        else
        {
            printf("buffer.startWrite failed\n");
            return -1;
        }
    }

    int process_readRequestReadSocket(ProcessResult &result)
    {        
        if(readRequest() != 0)
        {
            closeResult(result);
            return -1;
        }
        
        int parseRequestResult = parseRequest();

        if(parseRequestResult == 0)
        {
			bytesToSend = fileSize(fileName);

            if(bytesToSend < 0)
            {
                printf("fileSize failed\n");
                closeResult(result);
                return -1;
            }

			fileFd = open(fileName, O_NONBLOCK | O_RDONLY);
            
            if(fileFd < 0)
            {
				perror("open failed");
                closeResult(result);
                return -1;
            }

            if(createResponse() != 0)
            {
                closeResult(result);
                return -1;
            }

			result.editFd = clientSocketFd;
			result.editFdEvents = EPOLLOUT;
            
            result.action = ProcessResult::Action::editPoll;
            
            state = State::sendResponse;
        }
        else if(parseRequestResult == -2)
        {
            closeResult(result);
            return -1;
        }

		return 0;
    }

    int process_sendResponseSendData(ProcessResult &result)
    {
        void *data;
        int size;

        if(buffer.startRead(data, size))
        {
            int bytesWritten = write(clientSocketFd, data, size);

            if(bytesWritten <= 0)
            {
                if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR)
                {
                    closeResult(result);
                    return -1;
                }
            }
            else
            {
                buffer.endRead(bytesWritten);

                if(bytesWritten == size)
                {
                    state = State::sendFile;
                }
            }

            result.action = ProcessResult::Action::none;
            return 0;
        }
        else
        {
            closeResult(result);
            return -1;
        }
    }

    int process_sendFile(ProcessResult &result)
    {		
		int bytesWritten = sendfile(clientSocketFd, fileFd, &filePosition, bytesToSend);
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
                closeResult(result);
                return -1;
            }
            
        }

		bytesToSend -= bytesWritten;

		if(bytesToSend == 0)
		{
			closeResult(result);
			return 0;
		}

		result.action = ProcessResult::Action::none;
		return 0;
    }
    
    int process(int fd, int events, ProcessResult &result) override
    {
        if(state == State::readRequest && fd == clientSocketFd && (events & EPOLLIN))
		{
            return process_readRequestReadSocket(result);
		}
        if(state == State::sendResponse && fd == clientSocketFd && (events & EPOLLOUT))
        {
            return process_sendResponseSendData(result);
        }
		if(state == State::sendFile && fd == clientSocketFd && (events & EPOLLOUT))
		{
            return process_sendFile(result);
		}

		printf("invalid process call\n");
		result.action = ProcessResult::Action::none;
		return 0;
    }
    
    void down()
    {
        if(clientSocketFd > 0)
        {
            close(clientSocketFd);
            clientSocketFd = -1;
        }
        if(fileFd > 0)
        {
            close(fileFd);
            fileFd = -1;
		}
		state = State::invalid;
		return;
    }
    
	int getFds(int *fds) override
	{
		int index = 0;
		if(clientSocketFd > 0)
		{
			fds[index] = clientSocketFd;
			++index;
		}
		return index;
	}

protected:
    static const int REQUEST_BUFFER_SIZE = 1000;
    static const int MAX_FILE_NAME = 300;

    enum class State { readRequest, sendResponse, sendFile, invalid };

	State state = State::invalid;

	int clientSocketFd = -1;
	int fileFd = -1;

	long long int bytesToSend = 0;
	off_t filePosition = 0;

    TransferRingBuffer buffer;

    char fileName[MAX_FILE_NAME + 1];
    int rootFolderLength = 0;
    char urlBuffer[REQUEST_BUFFER_SIZE];
};

#endif
