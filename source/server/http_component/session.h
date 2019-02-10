#ifndef HTTPD_SESSION_H
#define HTTPD_SESSION_H

#include <map.h>

typedef struct session {
    long time_stamp;
    int session_id;
    int is_time_out;
    map_str_t parameters;
} session;

char *get_parameter (session *sessions, char *paramter);

void set_parameter (session *sessions, char *paramter, char *value);

#endif //HTTPD_SESSION_H
