#ifndef _LOG_H_
#define _LOG_H_

class ServerParameters;

class Log
{
public:
    enum class Level
    {
        debug = 1, info = 2, warning = 3, error = 4
    };

    enum class Type
    {
        stdout, mmap
    };

    virtual ~Log() { }

    virtual int init(ServerParameters *params) = 0;

    virtual void debug(const char* format, ...) = 0;
    virtual void info(const char* format, ...) = 0;
    virtual void warning(const char* format, ...) = 0;
    virtual void error(const char* format, ...) = 0;

protected:

    Level level = Level::info;
};


#endif
