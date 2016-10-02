project (http_server)

set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("." "../cpp")

set(SOURCE 
	main.cpp 
	Executor.h
	ServerExecutor.h
    SslServerExecutor.h
    RequestExecutor.h
    FileExecutor.h
    Server.h
    ServerParameters.h
    ProcessResult.h
    Log.h LogStdout.h
    PollData.h
    ExecutorData.h
    UwsgiExecutor.h
    PollLoopBase.h PollLoop.h
    ServerBase.h
    ExecutorType.h
    NewFdExecutor.h
    SslRequestExecutor.h
    SslFileExecutor.h
    SslUwsgiExecutor.h
    percent_decode.h percent_decode.c
    ../cpp/FileUtils.h ../cpp/FileUtils.cpp
    ../cpp/NetworkUtils.h ../cpp/NetworkUtils.cpp
    ../cpp/TransferRingBuffer.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -O0 -g -pthread")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -O2")

add_executable(http_server ${SOURCE})

target_link_libraries(http_server ssl)

