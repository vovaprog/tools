#ifndef UWSGI_EXECUTOR_H
#define UWSGI_EXECUTOR_H

#include <sys/epoll.h>
#include <errno.h>

#include <ExecutorData.h>
#include <ProcessResult.h>
#include <FileUtils.h>
#include <NetworkUtils.h>

class UwsgiExecutor: public Executor {
public:

	int up(ExecutorData &data) override
	{
		data.fd1 = socketConnect("127.0.0.1", 7070);

		if(data.fd1 < 0)
		{
			data.ctx->log->error("socketConnect failed\n");
			return -1;
		}

		if(setNonBlock(data.fd1) != 0)
		{
			data.ctx->log->error("setNonBlock failed: %s\n", strerror(errno));
			close(data.fd1);
			data.fd1 = -1;
			return -1;
		}

		data.state = ExecutorData::State::forwardRequest;

		return 0;
	}

	int process(ExecutorData &data, int fd, int events, ProcessResult &result) override
	{
		if(data.state == ExecutorData::State::forwardRequest && fd == data.fd1 && (events & EPOLLOUT))
		{
			return process_forwardRequest(data, result);
		}
		if(data.state == ExecutorData::State::forwardResponse && fd == data.fd1 && (events & EPOLLIN))
		{
			return process_forwardResponseRead(data, result);
		}
		if(data.state == ExecutorData::State::forwardResponse && fd == data.fd0 && (events & EPOLLOUT))
		{
			return process_forwardResponseWrite(data, result);
		}
		if(data.state == ExecutorData::State::forwardResponseOnlyWrite && fd == data.fd0 && (events & EPOLLOUT))
		{
			return process_forwardResponseOnlyWrite(data, result);
		}

		data.ctx->log->warning("invalid process call\n");
		result.action = ProcessResult::Action::none;
		return 0;
	}


protected:

	int process_forwardRequest(ExecutorData &data, ProcessResult &result)
	{
		void *p;
		int size;

		if(data.buffer.startRead(p, size))
		{
			char temp[10000];
			strncpy(temp, (char*)p, 400);
			printf("<%s>\n",temp);fflush(stdout);


			int bytesWritten = write(data.fd1, p, size);

			if(bytesWritten <= 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					result.action = ProcessResult::Action::none;
					return 0;
				}
				else
				{
					data.ctx->log->error("sendfile failed: %s\n", strerror(errno));
					result.closeResult();
					return -1;
				}
			}

			data.buffer.endRead(bytesWritten);

			if(bytesWritten == size)
			{
				data.buffer.clear();

				data.state = ExecutorData::State::forwardResponse;

				result.action = ProcessResult::Action::editPoll;
				result.editFd0Events = EPOLLOUT;
				result.editFd1Events = EPOLLIN;
				return 0;
			}

			result.action = ProcessResult::Action::none;
			return 0;
		}
		else
		{
			data.ctx->log->error("buffer.startRead failed\n");
			result.closeResult();
			return -1;
		}
	}

	int process_forwardResponseRead(ExecutorData &data, ProcessResult &result)
	{
		void *p;
		int size;

		if(data.buffer.startWrite(p, size))
		{
			int bytesRead = read(data.fd1, p, size);

			if(bytesRead <= 0)
			{
				if(bytesRead == 0)
				{
					perror("read failed");
					close(data.fd1);
					data.fd1 = -1;
					data.state = ExecutorData::State::forwardResponseOnlyWrite;
					result.action = ProcessResult::Action::none;
					return 0;
				}
				else if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					result.action = ProcessResult::Action::none;
					return 0;
				}
				else
				{
					data.ctx->log->error("read failed: %s\n", strerror(errno));
					result.closeResult();
					return -1;
				}
			}
			else
			{
				data.buffer.endWrite(bytesRead);
				result.action = ProcessResult::Action::none;
				return 0;
			}
		}
		else
		{
			result.action = ProcessResult::Action::none;
			return 0;
		}
	}

	int process_forwardResponseWrite(ExecutorData &data, ProcessResult &result)
	{
		void *p;
		int size;

		if(data.buffer.startRead(p, size))
		{
			int bytesWritten = write(data.fd0, p, size);

			if(bytesWritten <= 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					result.action = ProcessResult::Action::none;
					return 0;
				}
				else
				{
					data.ctx->log->error("write failed: %s\n", strerror(errno));
					result.closeResult();
					return -1;
				}
			}

			data.buffer.endRead(bytesWritten);
		}

		result.action = ProcessResult::Action::none;
		return 0;
	}

	int process_forwardResponseOnlyWrite(ExecutorData &data, ProcessResult &result)
	{
		void *p;
		int size;

		if(data.buffer.startRead(p, size))
		{
			int bytesWritten = write(data.fd0, p, size);

			if(bytesWritten <= 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					result.action = ProcessResult::Action::none;
					return 0;
				}
				else
				{
					data.ctx->log->error("write failed: %s\n", strerror(errno));
					result.closeResult();
					return -1;
				}
			}

			data.buffer.endRead(bytesWritten);

			if(bytesWritten == size && !data.buffer.readAvailable())
			{
				result.action = ProcessResult::Action::removeExecutor;
				return 0;
			}
			else
			{
				result.action = ProcessResult::Action::none;
				return 0;
			}
		}
		else
		{
			result.action = ProcessResult::Action::removeExecutor;
			return 0;
		}
	}
};

#endif
