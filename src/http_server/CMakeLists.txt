cmake_minimum_required(VERSION 2.8)

project (http_server)

set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("." "./log" "./utils" "./executors" "./tinyxml2")

set(SOURCE 
    main.cpp
    Server.h Server.cpp
    ServerParameters.h ServerParameters.cpp
    ProcessResult.h
    ./log/Log.h ./log/LogStdout.h ./log/LogMmap.h ./log/LogBase.h
    PollData.h
    ExecutorData.h
    executors/UwsgiExecutor.h
    PollLoopBase.h PollLoop.h
    ServerBase.h
    ExecutorType.h
    UwsgiApplicationParameters.h
    ./executors/Executor.h ./executors/Executor.cpp
    ./executors/ServerExecutor.h ./executors/ServerExecutor.cpp
    ./executors/SslServerExecutor.h
    ./executors/RequestExecutor.h ./executors/RequestExecutor.cpp
    ./executors/FileExecutor.h ./executors/FileExecutor.cpp
    ./executors/NewFdExecutor.h ./executors/NewFdExecutor.cpp
    ./executors/SslRequestExecutor.h
    ./executors/SslFileExecutor.h
    ./executors/SslUwsgiExecutor.h
    ./utils/percent_decode.h ./utils/percent_decode.c
    ./utils/FileUtils.h ./utils/FileUtils.cpp
    ./utils/NetworkUtils.h ./utils/NetworkUtils.cpp
    ./utils/TransferRingBuffer.h
    ./utils/TransferBuffer.h
    ./utils/TimeUtils.h ./utils/TimeUtils.cpp
    ./tinyxml2/tinyxml2.h ./tinyxml2/tinyxml2.cpp)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -pthread")


add_executable(http_server ${SOURCE})

target_link_libraries(http_server ssl crypto)

