#include <signal.h>

#include <Server.h>
#include <Log.h>


Server srv;
std::atomic_bool runFlag;

static void sig_int_handler(int i)
{
    printf("sig int\n");

    runFlag.store(false);
}


int main(int argc, char** argv)
{
    printf("USAGE: http_server config_file_name (default: ./config.xml)\n");

    const char *fileName = "./config.xml";

    if(argc > 1)
    {
        fileName = argv[1];
    }

    runFlag.store(true);
    signal(SIGINT, sig_int_handler);

    ServerParameters params;
    if(params.load(fileName) != 0)
    {
        return -1;
    }
   
    if(srv.start(params) != 0)
    {
        return -1;
    }

    while(runFlag.load())
    {
        srv.logStats();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }

    srv.stop();

    return 0;
}
