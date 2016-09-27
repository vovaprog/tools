
#include <ExecutorData.h>
#include <ProcessResult.h>
#include <FileUtils.h>

class FileExecutor: public Executor {
public:

	int up(ExecutorData &data)
	{
		data.bytesToSend = fileSize(data.ctx->fileNameBuffer);

		if(data.bytesToSend < 0)
		{
			data.ctx->log->info("fileSize failed: %s\n", strerror(errno));
			closeResult(result);
			return -1;
		}

		data.fd1 = open(ctx->fileNameBuffer, O_NONBLOCK | O_RDONLY);

		if(data.fd1 < 0)
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

		return 0;
	}

	int process(ExecutorData &data, int fd, int events, ProcessResult &result) override
	{
		if(data.state == ExecutorData::State::sendResponse && fd == clientSocketFd && (events & EPOLLOUT))
		{
			return process_sendResponseSendData(data, result);
		}
		if(data.state == ExecutorData::State::sendFile && fd == clientSocketFd && (events & EPOLLOUT))
		{
			return process_sendFile(data, result);
		}
	}

protected:

	int process_sendResponseSendData(ExecutorData &data, ProcessResult &result)
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

	int process_sendFile(ExecutorData &data, ProcessResult &result)
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
};
