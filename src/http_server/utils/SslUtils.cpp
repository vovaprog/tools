#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>

#include <Log.h>

static unsigned long id_function(void)
{
  return (unsigned long)pthread_self();
}

int initSsl(SSL_CTX* &globalSslCtx, Log *log)
{
    SSL_load_error_strings();
    SSL_library_init();

    globalSslCtx = SSL_CTX_new(SSLv23_method());
    if(globalSslCtx == NULL)
    {
        log->error("SSL_CTX_new failed\n");
        return -1;
    }

    //BIO* errBio = BIO_new_fd(2, BIO_NOCLOSE);

    if(SSL_CTX_use_certificate_file(globalSslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
    {
        log->error("SSL_CTX_use_certificate_file failed\n");
        SSL_CTX_free(globalSslCtx);
        return -1;
    }

    if(SSL_CTX_use_PrivateKey_file(globalSslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
    {
        log->error("SSL_CTX_use_PrivateKey_file failed\n");
        SSL_CTX_free(globalSslCtx);
        return -1;
    }

    if(SSL_CTX_check_private_key(globalSslCtx) <= 0)
    {
        log->error("SSL_CTX_check_private_key\n");
        SSL_CTX_free(globalSslCtx);
        return -1;
    }

	//CRYPTO_set_id_callback(id_function);
	//CRYPTO_THREADID_set_callback(id_function);


    log->info("ssl inited\n");

    return 0;
}

int destroySsl(SSL_CTX* &globalSslCtx)
{
    SSL_CTX_free(globalSslCtx);

    return 0;
}

