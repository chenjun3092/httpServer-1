#include "server.h"
#include "tool.h"
#include "log.h"
#include "init_config.h"
#include "parse_json.h"
#include "post/post_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/thread.h>
#include <signal.h>
#include "get/get_handler.h"
#include "http_util.h"
#include "http_component/cookie.h"
#include "../module/markdown_src/markdown_parser/markdown_parser.h"
#include "./http_component/session.h"
#include "mtime.h"


/*服务器全局配置*/
server_config_package *p = NULL;

void init_resp (response_struct *resp) {
    memset(resp->cookie, '\0', 256);
    memset(resp->type, '\0', 64);
    memset(resp->host, '\0', 128);
    memset(resp->desc, '\0', 32);
    resp->len = 0;
    resp->no = 0;
}

void send_500file (struct bufferevent *bev) {
    char buf[128];
    /*发送预置的500页面*/
    sprintf(buf, p->error_page, "500.html");
    send_file(bev, buf);
}

void send_404file (struct bufferevent *bev) {
    char buf[128];
    /*发送预置的404页面*/
    sprintf(buf, p->error_page, "404.html");
    send_file(bev, buf);
}

/**
 * @param bev
 * @param filename 用户需要访问的服务器中的文件的文件名,不包括JavaWeb的转发路径
 */
void send_file (struct bufferevent *bev, const char *filename) {
    char error_buf[128] = {0};
    char *dot = strrchr(filename, '.');
    if (dot && !strcmp(dot, ".md")) {
        /*do_parser(filename);*/
        strcpy(dot, ".html");
    }
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        /*在服务器中找不到给定文件名的文件*/
        sprintf(error_buf, "%s %s", filename, "发生404错误");
        write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
        send_404file(bev);
    } else {
        int flags;
        /**设置文件的非阻塞读*/
        flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(STDIN_FILENO, F_SETFL, flags);
        char buf[8192] = {0};
        int len = 0;
        while ((len = (int) read(file_fd, buf, 8192 * sizeof(char))) > 0) {
            bufferevent_write(bev, buf, (size_t) len);
            memset(buf, '\0', strlen(buf));
        }
        close(file_fd);
        /**读取文件出现错误*/
        if (len == -1) {
            sprintf(error_buf, "%s %s", filename, "发生500错误");
            write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
            send_500file(bev);
        }
    }
}

/**
 * 向浏览器发送响应头信息
 * @param bev
 * @param resp 响应头
 */
void send_respond_head (struct bufferevent *bev, struct response_struct resp) {
    char buf[2048] = {0};
    /**状态行*/
    sprintf(buf, "HTTP/1.1 %d %s\r\n", resp.no, resp.desc);
    /**消息报头*/
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", resp.type);
    /**消息的长度*/
    sprintf(buf + strlen(buf), "Content-Length:%ld\r\n", resp.len);

    if (resp.no == 301) {
        sprintf(buf + strlen(buf), "Location:http://%s\r\n", resp.host);
    } else if (resp.no == 302) {
        /**用于被转发到tomcat的JavaWeb请求*/
        sprintf(buf + strlen(buf), "Location:%s\r\n", resp.host);
    }
    strcpy(buf + strlen(buf), "Connection: close\r\n");
    strcpy(buf + strlen(buf), "Server: helloServer\r\n");
    if (strlen(resp.cookie)) {
        sprintf(buf + strlen(buf), "Set-Cookie: %s\r\n", resp.cookie);
    }
    strcpy(buf + strlen(buf), "\r\n");
    bufferevent_write(bev, buf, strlen(buf));
}

/**
 *
 * @param bev
 * @param dirname 需要遍历的目录名
 */
void send_directory (struct bufferevent *bev, const char *dirname) {
    char buf[4096] = {0};
    char path[4096] = {0};
    char enstr[1024] = {0};
    /**拼接成目录头*/
    sprintf(buf, DIR_NAME, dirname);
    sprintf(buf + strlen(buf), DIR_CUR_NAME, dirname);
    struct dirent **ptr = NULL;
    /**以字母序排列dirname目录中的文件和子目录*/
    int num = scandir(dirname, &ptr, NULL, alphasort);
    if (num <= 0) {
        char error_buf[128];
        sprintf(error_buf, "%s %s", dirname, "文件夹打开失败");
        write_log(ALERT_L, getpid(), __FUNCTION__, __LINE__, error_buf);
        send_500file(bev);
    } else {
        /**
         * 将目录中的文件和子目录拼接成html表格
         */
        int i;
        for (i = 0; i < num; ++i) {
            char *name = ptr[i]->d_name;
            sprintf(path, "%s/%s", dirname, name);
            struct stat st;
            stat(path, &st);
            encode_str(enstr, sizeof(enstr), name);
            /**如果是普通文件*/
            if (S_ISREG(st.st_mode)) {
                sprintf(buf + strlen(buf),
                        REG_PATH,
                        enstr, name, (long) st.st_size);
            }
                /**如果是子目录*/
            else if (S_ISDIR(st.st_mode)) {
                sprintf(buf + strlen(buf),
                        DIR_PATH,
                        enstr, name, (long) st.st_size);
            }
            bufferevent_write(bev, buf, strlen(buf));
            free(ptr[i]);
            memset(buf, 0, sizeof(buf));

        }
        /**发送由子目录和普通文件元素构成的html表格*/
        sprintf(buf + strlen(buf), END_TABLE);
        bufferevent_write(bev, buf, strlen(buf));
    }
    if (ptr != NULL) {
        free(ptr);
    }
}

/**
 * 对send_directory 的一个包装函数
 * @param bev
 * @param file  目录名
 * @param host 当前httpserver运行的端口号
 */
void send_directory_ (struct bufferevent *bev, char *file, char *host, response_struct resp1) {
    size_t len = strlen(file);
    char error_buf[128] = {0};
    char t = file[len - 1];
    /**
     * 当确定请求的路径: http://server_path:port/dir 是一个目录时，补齐请求路径 .../dir 之后的 /
     * 并重定向到补齐之后的路径
     */
    struct response_struct resp;

    init_resp(&resp);
    if (strlen(resp1.cookie)) {
        strcpy(resp.cookie, resp1.cookie);
    }
    strcpy(resp.type, "text/html; charset=utf-8");
    resp.len = -1;
    if (t != '/') {
        /**通过http response code 产生重定向*/
        file[len] = '/';
        file[len + 1] = '\0';
        strcpy(host + strlen(host), file);
        strcpy(resp.host, host);
        sprintf(error_buf, "%s %s", host, "产生了301重定向");
        write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
        resp.no = 301;
        strcpy(resp.desc, "Moved Permanently");
        send_respond_head(bev, resp);
    } else {
        send_respond_head(bev, resp);
    }
    send_directory(bev, file);

}

/**实现一个简单的负载均衡*/
/**
 *
 * @return  当前被选中的服务器编号
 */
int select_server () {
    srand(time(NULL));
    return rand() % p->s_num;
}


/**
 *
 * @param bev
 * @param filepath 待发送文件的目录
 * @param host client访问的路径
 * @param st  待发送文件的stat 结构
 * @param resp http的响应头，主要便于传递cookie
 */
void send_filedir (struct bufferevent *bev, char *filepath,
                   char *host, struct stat st, response_struct resp) {
    if (S_ISDIR(st.st_mode)) {
        send_directory_(bev, filepath, host, resp);
    } else if (S_ISREG(st.st_mode)) {
        send_respond_head(bev, resp);
        send_file(bev, filepath);
    }
}

/**
 *
 * @param bev
 * @param request  http get 请求头
 * @param path  http 请求的路径
 */
extern get_function get_func_array[];

void
do_http_get_handler (struct bufferevent *bev, char *request, char *path, char *pre_session, char *session_id,
                     map_str_t *request_head) {
    response_struct resp;
    init_resp(&resp);
    char error_buf[512] = {0};
    char page[512];
    char *host;
    int is_get_func = 0;
    /** 处理get方法的参数 ?ie=utf-8&f=8 */
    char *paramters = strchr(path, '?');
    if (paramters != NULL) {
        *paramters = '\0';
        paramters++;
        int i;
        int j;
        struct session *s;
        char *value;
        char key[16];
        while (true) {
            value = malloc(32);
            memset(key, '\0', 16);
            memset(value, '\0', 32);
            i = j = 0;
            while (*paramters != '=') {
                key[i++] = *paramters++;
            }
            paramters++;
            while (*paramters != '&' && *paramters != '\0') {
                value[j++] = *paramters++;
            }
            s = *(struct session **) map_get(&p->sessions, pre_session);
            set_parameter(s, key, value);
            map_set(&p->sessions, pre_session, s);
            if (*paramters == '\0') {
                break;
            }
            paramters++;
        }

    }
    /**
     * 设置会话cookie
     */

    strcpy(session_id + strlen(session_id), "; path=/; ");
    strcpy(resp.cookie, session_id);

    /**
    * 获取请求头中的 Host:
    */
    get_function gfunc;
    char *res;
    for (int i = 0;; ++i) {
        gfunc = get_func_array[i];
        if (!gfunc.func) {
            break;
        } else if (!strcmp(gfunc.func_name, path)) {
            char *(*func) (char *, void *) =  gfunc.func;
            res = func(pre_session, request);
            strcpy(resp.desc, OK);
            resp.no = 200;
            resp.len = strlen(res);
            strcpy(resp.type, "application/json");
            send_respond_head(bev, resp);
            bufferevent_write(bev, res, (size_t) resp.len);
            free(res);
            is_get_func = 1;
        }
    }


    if (!is_get_func) {
        host = get_headval(request_head, request, "Host");
        if (!host) {
            write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "请求头解析错误");
            fprintf(stderr, "请求头解析错误\n");
            fflush(stderr);
            exit(1);
        }


        strcpy(host + strlen(host), "/");
        decode_str(path, path);
        /**client 在服务器上实际访问的位置*/
        char *file = path + 1;
        if (!strcmp(path, "/")) {
            /**访问的位置为首页*/
            sprintf(file, p->static_path, p->index_html);
        } else if (p->redirect_site[0] != NULL && p->redirect_path[0] != NULL) {
            /**
             * 判断当前的请求是否是需要转发的http request 路径
             */
            size_t len_r;
            int i;
            for (i = 0; i < p->r_num; ++i) {
                len_r = strlen(p->redirect_path[i]);
                if (!strncmp(p->redirect_path[i], path, len_r)) {
                    char redirect_path[128] = {0};
                    /**重定向到tomcat运行的端口*/
                    sprintf(redirect_path, "%s/%s", p->redirect_site[i], file);
                    sprintf(error_buf, "%s %s", redirect_path, "产生了302重定向");
                    write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);

                    strcpy(resp.type, "text/html; charset=utf-8");
                    resp.len = -1;
                    strcpy(resp.host, redirect_path);
                    resp.no = 302;
                    strcpy(resp.desc, "Moved Permanently");
                    send_respond_head(bev, resp);
                }
            }
        }
        do {
            int is_bloading = 0;
            int i;
            for (i = 0; i < p->d_len; ++i) {
                /**是否是负载均衡的路径*/
                if (!strcasecmp(path, p->dynamic_file[i])) {
                    char *host_ = NULL;
                    host_ = findiport(host, 0);
                    *host_ = '\0';
                    host_ = host;
                    /*用随机的方法选择当前提供服务的服务器*/
                    int n = 0;
                    if (p->load_balancing) {
                        n = select_server();
                    }
                    is_bloading = 1;
                    /**将请求转发到负载均衡服务器*/
                    sprintf(host, "%s:%d%s", host_, p->load_servers[n], path);
                    sprintf(error_buf, "%s %s", host, "产生了301重定向");
                    write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
                    resp.no = 301;
                    strcpy(resp.desc, "Moved Permanently");
                    strcpy(resp.host, host);
                    resp.len = -1;
                    strcpy(resp.type, "text/html");
                    send_respond_head(bev, resp);
                }
            }
            if (!is_bloading) {
                struct stat st;
                int ret = stat(file, &st);
                char *dot;
                if (ret == -1) {
                    strcpy(page, file);
                    sprintf(file, p->static_path, page);
                    /**判断当前访问的文件是否位于static目录下(一般用于重要的静态页面)*/
                    if (!stat(file, &st)) {
                        strcpy(resp.desc, OK);
                        resp.no = 200;
                        strcpy(resp.type, get_file_type(file));
                        dot = strchr(file, '.');
                        if (dot && !strcmp(dot, ".md")) {
                            do_parser(file);
                            strcpy(dot, ".html");
                        }
                        stat(file, &st);
                        resp.len = st.st_size;
                        send_filedir(bev, file, host, st, resp);
                    } else {
                        /**当前文件不存在且不是任何转发路径*/
                        memset(error_buf, '\0', strlen(error_buf));
                        sprintf(error_buf, "%s %s", file, "发生404错误");
                        write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
                        resp.no = 404;
                        resp.len = -1;
                        strcpy(resp.desc, "Not Found");
                        strcpy(resp.type, "text/html; charset=utf-8");
                        send_respond_head(bev, resp);
                        send_404file(bev);
                    }
                } else {
                    /*访问公开目录下的文件*/
                    strcpy(resp.desc, OK);
                    resp.no = 200;
                    strcpy(resp.type, get_file_type(file));
                    dot = strchr(file, '.');
                    if (dot && !strcmp(dot, ".md")) {
                        do_parser(file);
                        strcpy(dot, ".html");
                    }
                    stat(file, &st);
                    resp.len = st.st_size;
                    send_filedir(bev, file, host, st, resp);
                }
            }
        } while (0);

    }

}

extern post_func post_func_array[];

/**
 *
 * @param bev
 * @param request http post 请求头
 * @param path  http 请求路径
 */
void do_http_post_handler (struct bufferevent *bev, char *request,
                           char *path, char *pre_session, char *session_id, map_str_t *request_head) {
    int i;
    post_func p;
    char *res;
    response_struct resp;
    init_resp(&resp);
    for (i = 0;; ++i) {
        p = post_func_array[i];
        if (!p.func) {
            return;
        } else if (!strcmp(p.func_name, path)) {
            char *(*func) (void *, char *) =  p.func;
            res = func(request, pre_session);
            strcpy(resp.desc, OK);
            resp.no = 200;
            strcpy(session_id + strlen(session_id), "; path=/; ");
            strcpy(resp.cookie, session_id);
            resp.len = strlen(res);
            strcpy(resp.type, "application/json");
            send_respond_head(bev, resp);
            bufferevent_write(bev, res, (size_t) resp.len);
            free(res);
            break;
        }
    }
}


/**
 * 处理http请求的核心函数
 * @param bev     char *content_type = get_headval(&request_head, request, "Content-Type");

 */
void handle_http_request (struct bufferevent *bev) {

    char request[4096] = {0};
    bufferevent_read(bev, request, sizeof(request));
    char method[12], path[1024], protocol[12];
    sscanf(request, "%[^ ] %[^ ] %[^ \r\n] ", method, path, protocol);
    char *session_id = NULL;
    map_str_t request_head;
    map_init(&request_head);
    char pre_session[512] = {0};
    char *cookie = get_headval(&request_head, request, "Cookie");
    if (cookie != NULL) {
        session_id = get_cookie(cookie, "uid");
        if (!session_id) {
            session_id = malloc(sizeof(char) * 128);
            set_cookie(session_id, "uid", NULL);
            char *t = session_id;
            int i = 0;
            while (*t != '=') {
                t++;
            }
            t++;
            while (*t != ';') {
                pre_session[i++] = *t;
                t++;
            }
            /**首次为一个session设置相关联的数据结构*/
            session *s = malloc(sizeof(session));
            s->session_id = rand_num();
            p->session_sum++;
            s->time_stamp = get_unix_timestamp();
            map_init(&s->parameters);
            map_set(&p->sessions, pre_session, s);
        } else {
            int i = 0;
            char *t = session_id;
            while (*t != '=') {
                t++;
            }
            t++;
            while (*t != ';' && *t != '\0') {
                pre_session[i++] = *t;
                t++;
            }
            struct session **ss = (struct session **) map_get(&p->sessions, pre_session);
            if (!ss) {
                session *s = malloc(sizeof(session));
                s->session_id = rand_num();
                p->session_sum++;
                s->time_stamp = get_unix_timestamp();
                map_init(&s->parameters);
                map_set(&p->sessions, pre_session, s);
            } else {
                /**更新unix时间戳*/
                (*ss)->time_stamp = get_unix_timestamp();
            }
        }

    }
    if (!strcasecmp(method, "GET")) {
        do_http_get_handler(bev, request, path, pre_session, session_id, &request_head);
    } else if (!strcasecmp(method, "POST")) {
        do_http_post_handler(bev, request, path, pre_session, session_id, &request_head);
    } else {
        //todo 未知的http请求方法
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "未知的http请求方法");
    }
    free(session_id);
    const char *key;
    map_iter_t iter = map_iter(&m);
    while ((key = map_next(&request_head, &iter))) {
        char *val_d = *map_get(&request_head, key);
        free(val_d);
    }
    map_deinit(&request_head);

}

void remove_timeout_session () {
    const char *key;
    map_iter_t iter = map_iter(&p->sessions);
    while ((key = map_next(&p->sessions, &iter))) {
        struct session *s = *map_get(&p->sessions, key);
        /**删除超过30分钟没有活动的session*/
        long cur_time = get_unix_timestamp();
        long s_time = s->time_stamp;
        /**设置30分钟过期时间*/
        if (cur_time - s_time > 1800) {
            write_log(ALERT_L, getpid(), __FUNCTION__, __LINE__, "移除了一个过期session");
            const char *key_ = NULL;
            iter = map_iter(&s->parameters);
            while ((key_ = map_next(&s->parameters, &iter))) {
                char *v = *map_get(&s->parameters, key_);
                if (v != NULL) {
                    free(v);
                }
                map_remove(&s->parameters, key_);
            }
            map_deinit(&s->parameters);
            free(s);
            map_remove(&p->sessions, key);
        }
    }
}

/**
 * 当libevent产生读事件后服务器自定义的请求处理函数
 * @param bev
 * @param arg : no use
 */
void read_cb (struct bufferevent *bev, void *arg) {
    remove_timeout_session();
    handle_http_request(bev);
}


void write_cb (struct bufferevent *bev, void *arg) {
    fflush(stdout);
}

void event_cb (struct bufferevent *bev, short events, void *arg) {
    evutil_socket_t fd = *((int *) arg);
    char error_buf[36] = {0};
    if (events & BEV_EVENT_EOF) {
        sprintf(error_buf, "套接字:%d 退出了连接\n", fd);
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, error_buf);
    } else if (events & BEV_EVENT_ERROR) {
        sprintf(error_buf, "套接字:%d 连接产生错误\n", fd);
        printf("%s\n", error_buf);
        fflush(stdout);
        write_log(ALERT_L, getpid(), __FUNCTION__, __LINE__, error_buf);
    }
    free((int *) arg);
    bufferevent_free(bev);
}


/**
 *
 * @param listener
 * @param fd   与客户端创建连接的套接字
 * @param addr
 * @param len
 * @param ptr
 */
void connect_init (
        struct evconnlistener *listener,
        evutil_socket_t fd,
        struct sockaddr *addr,
        int len, void *ptr) {
    struct event_base *base = (struct event_base *) ptr;
    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (bev == NULL) {
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "客户端创建连接失败");
        fprintf(stderr, "客户端创建连接失败\n");
        fflush(stderr);
    } else {
        char buf[128];
        sprintf(buf, "客户端创建连接创建监听成功,套接字为:%d", fd);
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, buf);
    }
    evutil_make_socket_nonblocking(fd);
    int *fd_ = malloc(sizeof(int));
    *fd_ = fd;
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, fd_);
    bufferevent_enable(bev, EV_READ);
    /**设置超时时间*/
    struct timeval r = {20, 0};
    struct timeval w = {40, 0};
    bufferevent_set_timeouts(bev, &r, &w);
}

/**
 * 根据json文件得到服务器的配置
 * @param json_path
 * @return  服务器的配置结构体
 */
server_config_package *init_server_config (const char *json_path) {
    server_config_package *package = parse_json(json_path);
    if (package == NULL) {
        write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "配置文件初始化失败");
        return NULL;
    } else {
        /*创建线程池(最小线程个数，最大线程个数，最大工作队列长度) */
        package->pool = threadpool_create(5, 40, 40);
        package->base = NULL;
        package->listener = NULL;
        return package;
    }
}

/**
 * 在服务器退出时，对libevent相应的资源进行回收
 * @param sig
 */
void signal_handler (int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGHUP:
        case SIGQUIT:
        case SIGINT: {
            struct event_base *base = p->base;
            struct evconnlistener *listener = p->listener;
            if (base != NULL && listener != NULL) {
                /*释放listener资源*/
                event_base_loopexit(base, NULL);
                evconnlistener_free(listener);
                event_base_free(base);
                const char *key = NULL;
                map_iter_t iter = map_iter(&p->sessions);
                while ((key = map_next(&p->sessions, &iter))) {
                    struct session *s = *map_get(&p->sessions, key);
                    const char *key_ = NULL;
                    iter = map_iter(&s->parameters);
                    while ((key_ = map_next(&s->parameters, &iter))) {
                        char *v = *map_get(&s->parameters, key_);
                        if (v != NULL) {
                            free(v);
                        }
                        map_remove(&s->parameters, key_);
                    }
                    map_deinit(&s->parameters);
                    free(s);
                    map_remove(&p->sessions, key);
                }
                map_deinit(&p->sessions);
                free(p);
                p = NULL;
            }
            exit(0);
        }
        default:
            exit(0);
    }
}

/**
 * 在预置的端口上开启http服务
 * @param listener
 * @param base
 * @param serv listener绑定的监听地址
 */
void event_init_listener (struct evconnlistener *listener, struct event_base *base, struct sockaddr_in serv) {

    listener = evconnlistener_new_bind(base, connect_init, base,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                       36, (struct sockaddr *) &serv, sizeof(serv));
    if (listener != NULL) {
        p->listener = listener;
        p->base = base;
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "绑定服务器端口成功");
    } else {
        /**
         * 启动配置文件中选定的备用端口
         */
        serv.sin_port = htons(p->al_port);
        listener = evconnlistener_new_bind(base, connect_init, base,
                                           LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                           36, (struct sockaddr *) &serv, sizeof(serv));
        if (listener != NULL) {
            p->listener = listener;
            p->base = base;
            write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "备用服务器端口启动成功");
        } else {
            write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "备用服务器端口启动失败");
            fprintf(stderr, "备用服务器端口启动失败\n");
            fflush(stderr);
            exit(1);

        }
    }
    /*设置信号处理函数*/
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    /**启动libevent的事件循环*/
    event_base_dispatch(base);
}

struct sockaddr_in init_serv () {
    /**初始化服务器监听的套接字结构体*/
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(p->listen_port);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    return serv;
}


/**
 * 服务器监听套接字的初始化工作
 */
void socket_serv_process () {
    struct sockaddr_in serv;
    evthread_use_pthreads();
    struct event_base *base;
    base = event_base_new();
    struct evconnlistener *listener;
    serv = init_serv();
    event_init_listener(listener, base, serv);

}

/**
 * 根据json配置文件初始化服务器,打开log文件
 * @param json_path json配置文件的目录
 */
void server_init (const char *json_path) {
    p = init_server_config(json_path);
    if (!p) {
        fprintf(stderr, "服务器初始化配置失败\n");
        fflush(stderr);
        exit(1);
    }
    /**初始化log文件*/
    if (open_log_fd(p->pool, p->log_path) != 0) {
        fprintf(stderr, "log文件初始化失败\n");
        fflush(stderr);
        exit(1);
    }
    map_init(&p->sessions);
    write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "初始化服务器成功");
    int ret = chdir(p->map_path);
    if (ret) {
        write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "工作目录切换失败");
        fprintf(stderr, "工作目录切换失败\n");
        fflush(stderr);
        exit(1);
    }
    socket_serv_process();
    exit(0);
}

