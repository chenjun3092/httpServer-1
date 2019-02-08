#ifndef HTTPSERV_INIT_CONFIG_H
#define HTTPSERV_INIT_CONFIG_H

#include "threadpool.h"

typedef struct server_config_package {
    struct evconnlistener *listener;
    struct event_base *base;
    int listen_port;
    const char *error_page;
    const char *map_path;
    const char *index_html;
    const char *log_path;
    const char *static_path;//静态文件的路径
    const char *dynamic_file[16];
    const char *redirect_path[16];//转发的路径
    const char *redirect_site[16];
    threadpool_t *pool;
    int load_servers[16];
    int d_len;
    int s_num;
    int r_num;//需要转发的路径条数
    int site_num;//转发网站的条数
    int al_port;//备用端口
    int load_balancing;
} server_config_package;
#endif //HTTPSERV_INIT_CONFIG_H
