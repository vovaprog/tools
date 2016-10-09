project (http_server)

set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("." "./utils" "./executors" "./tinyxml2")

set(SOURCE 
    main.cpp
    Server.h Server.cpp
    ServerParameters.h ServerParameters.cpp
    ProcessResult.h
    Log.h LogStdout.h LogMmap.h LogBase.h
    PollData.h
    ExecutorData.h
    executors/UwsgiExecutor.h
    PollLoopBase.h PollLoop.h
    ServerBase.h
    ExecutorType.h
    ./executors/Executor.h
    ./executors/ServerExecutor.h
    ./executors/SslServerExecutor.h
    ./executors/RequestExecutor.h
    ./executors/FileExecutor.h
    ./executors/NewFdExecutor.h
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -O0 -g -pthread")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -O2 -pthread")

add_executable(http_server ${SOURCE})

target_link_libraries(http_server ssl crypto)

