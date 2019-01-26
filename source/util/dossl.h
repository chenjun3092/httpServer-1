
#ifndef HTTPSERV_DOSSL_H
#define HTTPSERV_DOSSL_H

#include <openssl/ossl_typ.h>

void init_openssl ();

void cleanup_openssl ();

SSL_CTX *create_context ();

void configure_context (SSL_CTX *ctx);

#endif //HTTPSERV_DOSSL_H
