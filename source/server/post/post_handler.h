#ifndef HTTPD_POST_HANDLER_H
#define HTTPD_POST_HANDLER_H
struct post_func {
    char *func_name;
    void *func;
};
typedef struct post_func post_func;

char *post_login (void *arg, char *session_id);

char *post_sign_in (void *arg, char *session_id);

#endif //HTTPD_POST_HANDLER_H
