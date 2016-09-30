#ifndef _LOG_STDOUT_H_
#define _LOG_STDOUT_H_

#include <stdio.h>
#include <stdarg.h>
#include <Log.h>
#include <mutex>

class LogStdout: public Log
{
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
