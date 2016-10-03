#ifndef _LOG_MMAP_H_
#define _LOG_MMAP_H_

#include <stdio.h>
#include <stdarg.h>
#include <Log.h>
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <ServerParameters.h>
#include <TransferBuffer.h>

class LogMmap: public Log
{
public:
	int logFileSize;
	TransferBuffer buffer;
	char messageBuf[MSG_BUF_SIZE + 1];


	int openFile(char *fileName)
	{
		int fd = open("./log1.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if(fd < 0)
		{
			perror("open failed");
			return -1;
		}

		if(ftruncate(fd, logFileSize) != 0)
		{
			perror("ftruncate failed");
			return -1;
		}

		void *p = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
		if(p == MAP_FAILED)
		{
			perror("mmap failed");
			return -1;
		}

		buffer.init(p, logFileSize);

		return 0;
	}


	int init(ServerParameters &params)
	{
		Log::init(params.logLevel);

		logFileSize = params.logFileSize;

		if(openFile("./log1.txt") != 0)
		{
			return -1;
		}

		return 0;
	}


	void debug(const char* format, ...) override
	{
		if(level <= Level::debug)
		{
			std::lock_guard<std::mutex> lock(logMtx);

			void *data;
			int size;

			if(buffer.startWrite(data, size))
			{
				if(size > MESSAGE_SIZE)
				{
				}
			}

			printf("[DEBUG]   ");
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
		}
	}

	void info(const char* format, ...) override
	{
		if(level <= Level::info)
		{
			std::lock_guard<std::mutex> lock(logMtx);

			printf("[INFO]    ");
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
		}
	}

	void warning(const char* format, ...) override
	{
		if(level <= Level::warning)
		{
			std::lock_guard<std::mutex> lock(logMtx);

			printf("[WARNING] ");
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
		}
	}

	void error(const char* format, ...) override
	{
		std::lock_guard<std::mutex> lock(logMtx);

		printf("[ERROR]   ");
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}

private:
	std::mutex logMtx;
};


#endif
