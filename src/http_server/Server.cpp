#include <mutex>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>

#include <Server.h>


static std::recursive_mutex *sslMutexes = nullptr;
bool Server::sslInited = false;


static void sslIdCallback(CRYPTO_THREADID *pid)
{
    CRYPTO_THREADID_set_numeric(pid, (unsigned long)pthread_self());
//    printf("id_function2: %u\n", (unsigned int)pthread_self());
}


static void sslLockCallback(int mode, int type, const char *file, int line)
{
//    printf("lock callback: %d\n", type);

    (void)file;
    (void)line;
    if(mode & CRYPTO_LOCK) 
    {
        sslMutexes[type].lock();        
    }
    else 
    {
        sslMutexes[type].unlock();
    }
}


int Server::sslInit()
{
    if(!sslInited)
    {
        SSL_library_init();   
        SSL_load_error_strings();

        sslMutexes = new std::recursive_mutex[CRYPTO_num_locks()];

    	CRYPTO_THREADID_set_callback(sslIdCallback);
        CRYPTO_set_locking_callback(sslLockCallback);

        sslInited = true;
    }

    return 0;
}


void Server::cleanup()
{
    if(sslInited)
    {
        sslInited = false;

        CRYPTO_set_locking_callback(NULL);
        CRYPTO_set_id_callback(NULL);

        if(sslMutexes != nullptr)
        {
            delete[] sslMutexes;
            sslMutexes = nullptr;    
        }
    }
}


SSL_CTX* Server::sslCreateContext(Log *log)
{
    SSL_CTX *sslCtx = nullptr;

    sslCtx = SSL_CTX_new(SSLv23_method());
    if(sslCtx == NULL)
    {
        log->error("SSL_CTX_new failed\n");
        return nullptr;
    }

    if(SSL_CTX_use_certificate_file(sslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
    {
        log->error("SSL_CTX_use_certificate_file failed\n");
        SSL_CTX_free(sslCtx);
        return nullptr;
    }

    if(SSL_CTX_use_PrivateKey_file(sslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
    {
        log->error("SSL_CTX_use_PrivateKey_file failed\n");
        SSL_CTX_free(sslCtx);
        return nullptr;
    }

    if(SSL_CTX_check_private_key(sslCtx) <= 0)
    {
        log->error("SSL_CTX_check_private_key\n");
        SSL_CTX_free(sslCtx);
        return nullptr;
    }

    log->info("openssl context inited\n");

    return sslCtx;
}


void Server::sslDestroyContext(SSL_CTX *sslCtx)
{
    SSL_CTX_free(sslCtx);
}


