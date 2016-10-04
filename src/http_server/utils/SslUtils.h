#ifndef SSL_UTILS_H
#define SSL_UTILS_H

#include <openssl/ssl.h>

#include <Log.h>

int initSsl(SSL_CTX* &globalSslCtx, Log *log);
int destroySsl(SSL_CTX* &globalSslCtx);

#endif
