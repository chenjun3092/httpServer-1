#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "map.h"
#include <uuid/uuid.h>

/**
 *
 * @param ip_port host和port的组合体 类似于 127.0.0.1:9999
 * @param c c = 0,返回host; c =1,返回ip
 */
char *findiport (char *ip_port, int c) {
    if (ip_port != NULL) {
        switch (c) {
            case 0: {
                char *ip = strchr(ip_port, ':');
                return ip;
            }
            case 1: {
                char *port = strrchr(ip_port, ':');
                return port;
            }
            default: {
                return "";
            }
        }
    } else {
        return "";
    }
}

int hexit (char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}


void encode_str (char *to, int tosize, const char *from) {
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char *) 0) {
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }

    }
    *to = '\0';
}


void decode_str (char *to, char *from) {
    for (; *from != '\0'; ++to, ++from) {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {

            *to = hexit(from[1]) * 16 + hexit(from[2]);

            from += 2;
        } else {
            *to = *from;

        }

    }
    *to = '\0';

}


const char *get_file_type (const char *name) {
    char *dot;

    // 自右向左查找‘.’字符, 如不存在返回NULL
    dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".c") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".ico") == 0)
        return "image/png";
    if (strcmp(dot, ".txt") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

/**
 * @return 以特定的时间返回系统时间
 */
const char *get_datetime () {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *time_buf = malloc(sizeof(char) * 128);
    sprintf(time_buf, "%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
            tm.tm_sec);
    return time_buf;
}

/**
 * @return 返回系统时间中的hour
 */
int get_hour () {
    time_t t = time(NULL);
    struct tm tm_struct = *localtime(&t);
    int hour = tm_struct.tm_hour;
    return hour;
}

void set_cookie(char *cookies){
    uuid_t uu;
    uuid_generate(uu);
    memset(cookies, '\0', 128 * sizeof(char));
    strcpy(cookies, "uid=");
    for (int i = 0; i < 15; i++) {
        sprintf(cookies + strlen(cookies), "%02X-", uu[i]);
    }
    sprintf(cookies + strlen(cookies), "%02X", uu[15]);
    strcpy(cookies + strlen(cookies), "; path=/; Expires=Wed, 21 Oct 9999 12:59:59 GMT;");
    uuid_clear(uu);
}
/**
 * @return 返回系统时间中的天
 */
int get_day () {
    time_t t = time(NULL);
    struct tm tm_struct = *localtime(&t);
    int day = tm_struct.tm_yday;
    return day + 1;
}
/**
 *
 * @param reqhead 请求头
 * @param head_param 请求头中的键值对的键值
 */
char *parser_httphead (char *reqhead, char *head_param) {
    while (*reqhead != '\n') {
        reqhead++;
    }
    reqhead++;
    while (1) {
        if (!strncmp(reqhead, head_param, strlen(head_param))) {
            /*寻找需要特定的请求头*/
            char *tmp = strchr(reqhead, ':');
            tmp++;
            char *val = malloc(sizeof(char) * 1024);
            memset(val, '\0', 1024);
            int i = 0;
            while (*tmp != '\r' && *tmp != '\0') {
                tmp++;
                val[i++] = *tmp;
            }
            val[i - 1] = '\0';
            return val;
        } else {
            while (*reqhead != '\n' && *reqhead != '\0') {
                reqhead++;
            }
            if (*reqhead == '\0') {
                break;
            } else {
                reqhead++;
            }
        }
    }
    return NULL;
}

char *parse_cookies (char *c, char *h) {
    char *uid = malloc(sizeof(char) * 36);
    int is_finded = 0;
    while (1) {
        while (*c != ';') {
            c++;
            if (*c == '\0') {
                break;
            }
        }
        if (*c == '\0') {
            break;
        } else if (*c == ';') {
            c += 2;
            int i = 0;
            if (!strncmp(c, h, strlen(h))) {
                while (*c != ';' && *c != '\0') {
                    is_finded = 1;
                    uid[i++] = *c;
                    c++;
                }
            }
        }
    }
    if (is_finded)
        return uid;
    else
        return NULL;
}
/**
 *
 * @param m
 * @param request
 * @param param
 * @return 得到已经经过解析的键值对的值
 */
char *get_headval (map_str_t *m, char *request, char *param) {
    char **h;
    char *val = NULL;
    if ((h = map_get(m, param)) != NULL) {
        val = *h;
    } else {
        val = parser_httphead(request, param);
        map_set(m, param, val);
    }
    return val;
};