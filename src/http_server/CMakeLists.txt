project (http_server)

set(CMAKE_VERBOSE_MAKEFILE on)

include_directories(".")

set(SOURCE 
	main.cpp 
	Executor.h
	ServerExecutor.h
	ClientExecutor.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -g -O0")

add_executable(http_server ${SOURCE})


