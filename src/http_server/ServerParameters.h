#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

#include <Log.h>

struct ServerParameters {
    int maxClients = 0;
    int port = 0;
	Log::Level logLevel = Log::Level::info;
    char rootFolder[300];
};

#endif

