#include <map.h>
#include <stdlib.h>
#include "http_util.h"


/**
 *
 * @param reqhead 请求头
 * @param head_param 请求头中的键值对的键值
 */
char *parser_httphead (char *reqhead, char *head_param) {
    int is_find = 0;
    char *val = malloc(sizeof(char) * 256);
    memset(val, '\0', 256);
    while (*reqhead != '\n') {
        reqhead++;
    }
    reqhead++;
    while (1) {
        if (!strncmp(reqhead, head_param, strlen(head_param))) {
            /*寻找需要特定的请求头*/
            char *tmp = strchr(reqhead, ':');
            tmp++;

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
    return val;
}


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

/**
 * 获取post中的post负载信息
 * @param request
 * @param pay_load
 */
void get_payload (const char *request, char *pay_load) {
    int c = 0;
    int i = 0;
    while (request[i] != '\0') {
        if (request[i] == '\r' && request[i + 1] == '\n') {
            if (c == 1) {
                i += 2;
                break;
            } else {
                c = 0;
            }
        } else {
            c++;
        }
        i++;
    }
    int l = (int) strlen(request);
    c = 0;
    for (int j = i; j < l; ++j) {
        pay_load[c++] = request[j];
    }
}

