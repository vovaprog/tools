#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

#include <Log.h>
#include <vector>
#include <string>
#include <UwsgiApplicationParameters.h>

struct ServerParameters
{
    ServerParameters()
    {
        setDefaults();
    }

    void setDefaults()
    {
        maxClients = 1000;
        rootFolder = "./data";
        logFolder = "./log";
        threadCount = 1;
        httpPorts.clear();
        httpsPorts.clear();
        uwsgiApplications.clear();
        logLevel = Log::Level::info;
        logType = Log::Type::stdout;
        logFileSize = 1024 * 1024;
        logArchiveCount = 10;
        executorTimeoutMillis = 10000;
    }

    int load(const char *fileName);

    void writeToLog(Log *log);


    std::string rootFolder;
    std::string logFolder;

    int maxClients;
    int threadCount;
    int executorTimeoutMillis;

    Log::Level logLevel;
    Log::Type logType;
    int logFileSize;
    int logArchiveCount;

    std::vector<int> httpPorts;
    std::vector<int> httpsPorts;
    std::vector<UwsgiApplicationParameters> uwsgiApplications;
};

#endif

