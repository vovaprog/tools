#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

struct ServerParameters {
    int maxClients = 0;
    int port = 0;
    char rootFolder[300];
};

#endif

