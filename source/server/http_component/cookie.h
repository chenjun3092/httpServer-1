#ifndef HTTPD_COOKIE_H
#define HTTPD_COOKIE_H

void set_sid_cookie (char *cookie_session, char *h);

char *get_cookie (char *c, char *h);

void set_cookie (char *cookie, char *h, char *value);

void create_uuid (char *pre);

#endif //HTTPD_COOKIE_H
