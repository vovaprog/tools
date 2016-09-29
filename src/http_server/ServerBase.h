#ifndef SERVER_BASE_H
#define SERVER_BASE_H

class ServerBase {
public:

	virtual int createRequestExecutor(int fd) = 0;

};

#endif

