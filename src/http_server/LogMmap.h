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
	~LogMmap()
	{
		closeFile();
	}

	int init(ServerParameters &params)
	{
		Log::init(params.logLevel);

		logFileSize = params.logFileSize;
		logArchiveCount = params.logArchiveCount;

		rotate();

		return 0;
	}


	void debug(const char* format, ...) override
	{
		if(level <= Level::debug)
		{
			va_list args;
			va_start(args, format);

            writeLog("[DEBUG]  ", format, args);

			va_end(args);
		}
	}

	void info(const char* format, ...) override
	{
		if(level <= Level::info)
		{
			va_list args;
			va_start(args, format);
            writeLog("[INFO]   ", format, args);
			va_end(args);
		}
	}

	void warning(const char* format, ...) override
	{
		if(level <= Level::warning)
		{
			va_list args;
			va_start(args, format);
            writeLog("[WARNING]", format, args);
			va_end(args);
		}
	}

	void error(const char* format, ...) override
	{
		va_list args;
		va_start(args, format);
        writeLog("[ERROR]  ", format, args);
		va_end(args);
	}

protected:

	int openFile()
	{
		int fd = open("./log/http.log", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
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

		void *p = mmap(NULL, logFileSize, PROT_WRITE, MAP_SHARED, fd, 0);
		if(p == MAP_FAILED)
		{
			perror("mmap failed");
			return -1;
		}

		buffer.init(p, logFileSize);

		return 0;
	}

	void closeFile()
	{
		if(fd >= 0)
		{
			buffer.init(nullptr, 0);
			munmap(buffer.getDataPtr(), logFileSize);
			close(fd);
			fd = -1;
		}
	}

	int rotate()
	{
		closeFile();

		char fileName0[301];
		char fileName1[301];
		char *p0 = fileName0;
		char *p1 = fileName1;

		snprintf(p1, 300, "./log/http.log.%d", logArchiveCount);
		remove(p1);

		for(int i = logArchiveCount - 1;i>0;--i)
		{
			snprintf(p0, 300, "./log/http.log.%d", i);
			rename(p0, p1);
			std::swap(p0, p1);
		}

		strcpy(p0, "./log/http.log");
		rename(p0, p1);

		openFile();

		return 0;
	}

    int getCurrentTimeString(char *timeBuffer, int timeBufferSize)
    {
        time_t t;
        struct tm *timeinfo;

        time (&t);
        timeinfo = localtime(&t);
        if(strftime(timeBuffer,timeBufferSize,"%Y%m%d %H:%M:%S",timeinfo) > 0)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }

	void writeLog(const char *prefix, const char* format, va_list args)
	{
		std::lock_guard<std::mutex> lock(logMtx);

		void *data;
		int size;

		bool ret = buffer.startWrite(data, size);

		if(ret == false || (ret == true && size < maxMessageSize))
		{
			if(rotate() != 0)
			{
				return;
			}
			if(!buffer.startWrite(data,size))
			{
				return;
			}
			else if(size < maxMessageSize)
			{
				return;
			}
		}

        char timeBuffer[50];

        getCurrentTimeString(timeBuffer, 50);

        int prefixLength = sprintf((char*)data, "%s %s   ", prefix, timeBuffer);
		int bytesWritten = vsnprintf((char*)data + prefixLength, maxMessageSize - prefixLength, format, args);

		buffer.endWrite(prefixLength + bytesWritten);
	}

	std::mutex logMtx;

	int logFileSize = 0;
	int logArchiveCount = 0;

	TransferBuffer buffer;

	const int maxMessageSize = 512;

	int fd = -1;
};


#endif
