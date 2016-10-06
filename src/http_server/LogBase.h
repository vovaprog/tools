#ifndef _LOG_BASE_H_
#define _LOG_BASE_H_

#include <Log.h>
#include <ServerParameters.h>

class LogBase: public Log
{
public:
    int init(ServerParameters *params) override
    {
        this->level = params->logLevel;
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

    virtual void writeLog(const char *prefix, const char* format, va_list args) = 0;

};


#endif
