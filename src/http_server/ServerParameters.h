#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

#include <Log.h>
#include <vector>
#include <string>

struct ServerParameters
{
    int maxClients = 0;
    Log::Level logLevel = Log::Level::info;
    std::string rootFolder;
    //char wsgiApplications[MAX_APPLICATIONS + 1][20];
    int threadCount = 1;

    std::vector<int> httpPorts;
    std::vector<int> httpsPorts;

    std::vector<std::string> uwsgiApplications;
};

#endif

