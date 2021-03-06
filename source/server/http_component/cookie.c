#include "cookie.h"
#include <uuid/uuid.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "mtime.h"

void create_uuid (char *pre) {
    uuid_t uu;
    uuid_generate(uu);
    memset(pre, '\0', strlen(pre));
    uuid_unparse(uu, pre);
    uuid_clear(uu);
}

/**
 * 设置用户的session id
 * @param cookies  从http请求头中获取的cookie
 * @param h no use
 */
void set_sid_cookie (char *cookie_session, char *h) {
    uuid_t uu;
    uuid_generate(uu);
    strcpy(cookie_session, "uid=");
    uuid_unparse(uu, cookie_session + strlen(cookie_session));
    /** Expires=Wed, 21 Oct 9999 12:59:59 GMT;*/
    strcpy(cookie_session + strlen(cookie_session), "; path=/; ");
    /**
    char *t = get_cookie_time();
    strcpy(cookies + strlen(cookies), "Expires=");
    strncpy(cookies + strlen(cookies), t, strlen(t));
    strcpy(cookies + strlen(cookies), "; ");
    free(t);
    */
    uuid_clear(uu);
}

void refresh_sid_cookie (char *cookies) {
}

void set_cookie (char *cookie, char *h, char *value) {
    if (!strcmp(h, "uid")) {
        if (!value) {
            set_sid_cookie(cookie, h);
        } else {
            refresh_sid_cookie(cookie);
        }
    } else {
        //todo
    }
}

/**
 * 获取cookie对应的值
 * @param cookies
 * @param h
 * @return
 */
char *get_cookie (char *cookies, char *h) {
    int i = 0;
    char *tmp = cookies;
    char *res = NULL;
    if (!strncmp(tmp, h, strlen(h))) {
        res = malloc(sizeof(char) * 512);
        memset(res, '\0', 512 * sizeof(char));
        while (*tmp != ';' && *tmp != '\0') {
            res[i++] = *tmp;
            tmp++;
        }
        return res;
    } else {
        while (1) {
            while (*tmp != ';') {
                tmp++;
                if (*tmp == '\0') {
                    break;
                }
            }
            if (*tmp == '\0') {
                break;
            } else if (*tmp == ';') {
                tmp += 2;
                i = 0;
                if (!strncmp(tmp, h, strlen(h))) {
                    res = malloc(sizeof(char) * 512);
                    memset(res, '\0', 512 * sizeof(char));
                    while (*tmp != ';' && *tmp != '\0') {
                        res[i++] = *tmp;
                        tmp++;
                    }
                }
            }
        }
    }
    return res;
}
