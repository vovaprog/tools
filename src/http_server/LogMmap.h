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

class LogMmap: public Log
{
public:
	int logFileSize;
	int numberOfLogs;

	int init(ServerParameters &params)
	{
		int fd = open("./log1.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if(fd < 0)
		{
			perror("open");
			return -1;
		}

		int size = 1024 * 1024;

		if(ftruncate(fd, size) != 0)
		{
			perror("ftruncate");
			return -1;
		}

		void *p = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
		if(p == MAP_FAILED)
		{
			perror("mmap");
			return -1;
		}

		int off = 0;

		while(off < size)
		{
			char buf[1000];
			snprintf(buf, 1000, "offset: %d\n", off);
			int l = strlen(buf);
			if(off + l < size)
			{
				strcpy((char*)p + off, buf);
			}
			off += l;

			//std::this_thread::sleep_for()
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		return 0;
	}


	void debug(const char* format, ...) override
	{
		if(level <= Level::debug)
		{
			std::lock_guard<std::mutex> lock(logMtx);

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
