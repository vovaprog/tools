#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

#include <Log.h>
#include <vector>
#include <string>

struct ServerParameters
{
    ServerParameters()
    {
        setDefaults();
    }

    int load(const char *fileName);

    int maxClients;
    std::string rootFolder;
    int threadCount;

    std::vector<int> httpPorts;
    std::vector<int> httpsPorts;

    std::vector<std::string> uwsgiApplications;

    Log::Level logLevel;
    Log::Type logType;
    int logFileSize;
    int logArchiveCount;

    int executorTimeoutMillis;

    void setDefaults()
    {
        maxClients = 1000;
        rootFolder = "./data";
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

    void writeToLog(Log *log)
    {
        log->info("----- server parameters -----\n");
        log->info("maxClients: %d\n", maxClients);
        log->info("rootFolder: %s\n", rootFolder.c_str());
        log->info("threadCount: %d\n", threadCount);
        log->info("logLevel: %s\n", Log::logLevelString(logLevel));
        log->info("logType: %s\n", Log::logTypeString(logType));
        log->info("-----------------------------\n");
    }
};

#endif

