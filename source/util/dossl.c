
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <log.h>
#include <string.h>

void init_openssl () {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl () {
    EVP_cleanup();
}

SSL_CTX *create_context () {
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        char error_buf[48] = {0};
        strcpy(error_buf,"unable to create SSL context ");
        write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
        ERR_print_errors_fp(stderr);
        exit(1);
    }
    return ctx;
}

void configure_context (SSL_CTX *ctx) {
    SSL_CTX_set_ecdh_auto(ctx, 1);
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        char error_buf[48] = {0};
        strcpy(error_buf,"certificate init error");
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
        char error_buf[48] = {0};
        strcpy(error_buf,"privateKey init error");
        ERR_print_errors_fp(stderr);
        exit(1);
    }
}