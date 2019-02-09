
#ifndef HTTPD_HTTP_UTIL_H
#define HTTPD_HTTP_UTIL_H

#include "map.h"

char *parser_httphead (char *reqhead, char *head_param);

char *findiport (char *ip_port, int c);

char *get_headval (map_str_t *m, char *request, char *param);

void get_payload (const char *request, char *pay_load);

#endif //HTTPD_HTTP_UTIL_H
