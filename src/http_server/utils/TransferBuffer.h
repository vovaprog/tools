#ifndef TRANSFER_BUFFER_H
#define TRANSFER_BUFFER_H

class TransferBuffer
{
public:
	TransferBuffer(void *buf, int size): buf(buf), bufSize(size), readHead(0), writeHead(0)
    {
    }

	~TransferBuffer()
    {
    }

	TransferBuffer(const TransferBuffer &tm) = delete;
	TransferBuffer(TransferBuffer &&tm) = delete;
	TransferBuffer& operator=(const TransferBuffer &tm) = delete;
	TransferBuffer& operator=(TransferBuffer && tm) = delete;

	void init(void *buffer, int size)
	{
		buf = buffer;
		bufferSize = size;
		clear();
		return 0;
	}

    void clear()
    {
        writeHead = 0;
        readHead = 0;
    }

    bool startWrite(void* &data, int &size)
    {
		if(writeHead >= bufSize)
		{
			return false;
		}

		data = buf + writeHead;
		size = bufSize - writeHead;

		return true;
    }

    void endWrite(int size)
    {
        writeHead += size;
    }

    bool startRead(void* &data, int &size)
    {
        data = buf + readHead;
		size = writeHead - readHead;

		return (size > 0);
    }

    void endRead(int size)
    {
        readHead += size;
    }

	bool readAvailable()
	{
		void *data;
		int size;
		return startRead(data, size);
	}

protected:
    char *buf = nullptr;
    int readHead = 0, writeHead = 0, bufSize = 0;
};

#endif // TRANSFER_BUFFER_H
