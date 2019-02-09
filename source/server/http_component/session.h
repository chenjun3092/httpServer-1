#ifndef HTTPD_SESSION_H
#define HTTPD_SESSION_H

#include <map.h>

typedef struct session {
    int session_id;
    map_str_t parameters;
} session;

char *get_parameter (session *sessions, char *paramter);

void set_parameter (session *sessions, char *paramter, char *value);

#endif //HTTPD_SESSION_H
