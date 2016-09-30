#ifndef _LOG_H_
#define _LOG_H_

class Log
{
public:
    enum class Level
    {
        debug = 1, info = 2, warning = 3, error = 4
    };

    virtual void init(Level level)
    {
        this->level = level;
    }

    virtual void debug(const char* format, ...) = 0;
    virtual void info(const char* format, ...) = 0;
    virtual void warning(const char* format, ...) = 0;
    virtual void error(const char* format, ...) = 0;

protected:
    Level level = Level::info;
};


#endif
