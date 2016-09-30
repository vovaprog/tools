#ifndef SSL_REQUEST_EXECUTOR_H
#define SSL_REQUEST_EXECUTOR_H

#include <openssl/ssl.h>

#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>

class SslRequestExecutor: public Executor
{
public:
	int init(PollLoopBase *loop) override
	{
		this->loop = loop;
		log = loop->log;
		return 0;
	}

	int up(ExecutorData &data) override
	{
		if(sslInit(data) != 0)
		{
			return -1;
		}

		HandleHandshakeResult result = handleHandshake(data, true);

		if(result == HandleHandshakeResult::ok)
		{
			data.state = ExecutorData::State::readRequest;
		}
		else if(result == HandleHandshakeResult::again)
		{
			data.state = ExecutorData::State::sslHandshake;
		}
		else
		{
			return -1;
		}

		return 0;
	}

	int sslInit(ExecutorData &data)
	{
		SSL *ssl = SSL_new(loop->srv->globalSslCtx);

		if(ssl == NULL)
		{
			log->error("SSL_new failed\n");
			return -1;
		}

		if(SSL_set_fd(ssl, data.fd0) == 0)
		{
			log->error("SSL_set_fd failed\n");
			SSL_shutdown(ssl);
			SSL_free(ssl);
			return -1;
		}

		SSL_set_accept_state(ssl);

		data.ssl = ssl;

		return 0;
	}

	enum class HandleHandshakeResult { ok, error, again };

	HandleHandshakeResult handleHandshake(ExecutorData &data, bool firstTime)
	{
		int ret = SSL_do_handshake(data.ssl);
		if(ret == 1)
		{
			return HandleHandshakeResult::ok;
		}
		int err = SSL_get_error(data.ssl, ret);
		if(err == SSL_ERROR_WANT_WRITE)
		{
			if(firstTime)
			{
				loop->addPollFd(data, data.fd0, EPOLLOUT);
			}
			else
			{
				loop->editPollFd(data, data.fd0, EPOLLOUT);
			}
		}
		else if(err == SSL_ERROR_WANT_READ)
		{
			if(firstTime)
			{
				loop->addPollFd(data, data.fd0, EPOLLIN);
			}
			else
			{
				loop->editPollFd(data, data.fd0, EPOLLIN);
			}
		}
		else
		{
			log->error("SSL_do_handshake return %d error %d errno %d msg %s\n", ret, err, errno, strerror(errno));
			return HandleHandshakeResult::error;
		}

		return HandleHandshakeResult::again;
	}

	ProcessResult process(ExecutorData &data, int fd, int events) override
	{
		if(data.state == ExecutorData::State::sslHandshake && fd == data.fd0)
		{
			return process_handshake(data);
		}

		return ProcessResult::ok;
	}

protected:

	ProcessResult process_handshake(ExecutorData &data)
	{
		HandleHandshakeResult result = handleHandshake(data, false);

		if(result == HandleHandshakeResult::ok)
		{
			if(loop->editPollFd(data, data.fd0, EPOLLIN) != 0)
			{
				return ProcessResult::removeExecutor;
			}
			data.state = ExecutorData::State::readRequest;
			return ProcessResult::ok;
		}
		else if(result == HandleHandshakeResult::again)
		{
			return ProcessResult::ok;
		}
		else
		{
			return ProcessResult::removeExecutor;
		}
	}


	PollLoopBase *loop = nullptr;
	Log *log = nullptr;
};



#endif

