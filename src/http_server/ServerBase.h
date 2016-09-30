#ifndef SERVER_BASE_H
#define SERVER_BASE_H

#include <Log.h>

class ServerBase {
public:

	virtual int createRequestExecutor(int fd) = 0;

	Log *log = nullptr;
};

#endif

