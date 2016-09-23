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

class ClientExecutor: public Executor {
public:
    int init(int clientSocketFd)
    {
		cleanup();
        
        this->clientSocketFd = clientSocketFd;        
		bytesToSend = 0;
		filePosition = 0;
		requestBuffer.init(1000);

		state = State::waitRequest;

        return 0;
    }
    
    int readRequest()
    {
		void *data;
		int size;

        if(requestBuffer.startWrite(data, size))
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
				requestBuffer.endWrite(rd);
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
    
	int parseRequest()
	{
		strcpy(fileName, "/home/vlads/programs/tools/src/http_server/build/data/test.jpg");
		return 0;
	}

    int waitRequest_ReadSocket(ProcessResult &result)
    {        
        if(readRequest() != 0)
        {
            closeResult(result);
            return -1;
        }
        
        if(parseRequest() == 0)
        {
			bytesToSend = fileSize(fileName);

			fileFd = open(fileName, O_NONBLOCK | O_RDONLY);
            
            if(fileFd < 0)
            {
				perror("open failed");
                closeResult(result);
                return -1;
            }

			result.editFd = clientSocketFd;
			result.editFdEvents = EPOLLOUT;
            
            result.action = ProcessResult::Action::editPoll;
            
			state = State::sendFile;
        }

		return 0;
    }
    
    int sendFile_sendData(ProcessResult &result)
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
		if(state == State::waitRequest && fd == clientSocketFd && (events & EPOLLIN))
		{
			return waitRequest_ReadSocket(result);
		}
		if(state == State::sendFile && fd == clientSocketFd && (events & EPOLLOUT))
		{
			return sendFile_sendData(result);
		}
		if(state == State::sendFile && fd == fileFd && (events & EPOLLIN))
		{
			return sendFile_sendData(result);
		}

		printf("invalid process call\n");
		result.action = ProcessResult::Action::none;
		return 0;
    }
    
	void cleanup()
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
	enum class State { waitRequest, sendFile, invalid };

	State state = State::invalid;

	int clientSocketFd = -1;
	int fileFd = -1;

	long long int bytesToSend = 0;
	off_t filePosition = 0;

	TransferRingBuffer requestBuffer;
	char fileName[300];
};

#endif
