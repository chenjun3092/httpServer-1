
#ifndef HTTPSERV_TOOL_H
#define HTTPSERV_TOOL_H

#include <map.h>

int hexit (char c);

char *findiport (char *ip_port, int c);

void encode_str (char *to, int tosize, const char *from);

void decode_str (char *to, char *from);

const char *get_file_type (const char *name);

const char *get_datetime ();

int get_hour ();

int get_day ();

char *parser_httphead (char *reqhead, char *head_param);

char *get_headval (map_str_t *m, char *request, char *param);

void set_cookie (char *cookies);

char *parse_cookies (char *c, char *h);

#endif //HTTPSERV_TOOL_H
