#ifndef _LOG_STDOUT_H_
#define _LOG_STDOUT_H_

#include <stdio.h>
#include <stdarg.h>
#include <Log.h>
#include <LogBase.h>
#include <mutex>
#include <TimeUtils.h>

class LogStdout: public LogBase
{
protected:

	void writeLog(const char *prefix, const char* format, va_list args) override
	{
		std::lock_guard<std::mutex> lock(logMtx);

		char timeBuffer[50];

		getCurrentTimeString(timeBuffer, 50);

		printf("%s %s   ", prefix, timeBuffer);
		vprintf(format, args);
	}


private:
    std::mutex logMtx;
};


#endif
