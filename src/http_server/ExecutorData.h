

class Executor;

struct ExecutorData {
	~ExecutorData()
	{
		down();
	}

	int up()
	{
		down();

		this->clientSocketFd = clientSocketFd;
		bytesToSend = 0;
		filePosition = 0;
		buffer.init(REQUEST_BUFFER_SIZE);

		state = State::readRequest;

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

	static const int REQUEST_BUFFER_SIZE = 1000;

	enum class State { readRequest, sendResponse, sendFile, invalid };

	Executor *pExecutor = nullptr;

	State state = State::invalid;

	int fd0 = -1;
	int fd1 = -1;
	int pollIndexFd0 = -1;
	int pollIndexFd1 = -1;

	long long int bytesToSend = 0;
	off_t filePosition = 0;

	TransferRingBuffer buffer;

	ServerContext *ctx = nullptr;
};
