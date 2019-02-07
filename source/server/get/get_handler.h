#ifndef HTTPD_GET_HANDLER_H
#define HTTPD_GET_HANDLER_H
typedef struct getfunc {
    char *func_name;
    void *func;
} get_function;

char *login (void *arg);

char *sign_in (void *arg);

#endif //HTTPD_GET_HANDLER_H
