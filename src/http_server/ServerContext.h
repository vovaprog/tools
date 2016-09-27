#ifndef SERVER_CONTEXT_H
#define SERVER_CONTEXT_H

#include <Log.h>
#include <ServerParameters.h>

struct ServerContext {

	Log *log = nullptr;

	static const int MAX_FILE_NAME = 300;

	char fileNameBuffer[MAX_FILE_NAME + 1];
	int rootFolderLength = 0;

	ServerParameters parameters;
};

#endif
