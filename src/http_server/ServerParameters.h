#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

#include <Log.h>

struct ServerParameters {
	static const int MAX_APPLICATIONS = 5;

    int maxClients = 0;
    int port = 0;
	Log::Level logLevel = Log::Level::info;
    char rootFolder[300];
	char wsgiApplications[MAX_APPLICATIONS + 1][20];
};

#endif

