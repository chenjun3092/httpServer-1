#include "server.h"
#include "tool.h"
#include "log.h"
#include "init_config.h"
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
#include "parse_json.h"
#include <event2/thread.h>
#include <signal.h>

/*服务器全局配置*/
server_config_package *p = NULL;

void init_resp (response_struct *resp) {
    memset(resp->cookie, '\0', 256);
    memset(resp->desc, '\0', 32);
    memset(resp->type, '\0', 32);
    memset(resp->host, '\0', 128);
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
        while ((len = (int) read(file_fd, buf, sizeof(buf))) > 0) {
            bufferevent_write(bev, buf, strlen(buf));
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
    strcpy(buf + strlen(buf), "Connection: keep-alive\r\n");
    strcpy(buf + strlen(buf), "Server: helloServer\r\n");
    if (strlen(resp.cookie) != 0) {
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
    struct dirent **ptr;
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
            memset(buf, 0, sizeof(buf));
        }
        /**发送由子目录和普通文件元素构成的html表格*/
        sprintf(buf + strlen(buf), END_TABLE);
        bufferevent_write(bev, buf, strlen(buf));
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
    if (strlen(resp1.cookie) != 0) {
        strcpy(resp.cookie, resp1.cookie);
    }
    resp.no = 301;
    strcpy(resp.type, "text/html; charset=utf-8");
    resp.len = -1;
    strcpy(resp.desc, "Moved Permanently");
    strcpy(resp.host, host);
    if (t != '/') {
        /**通过http response code 产生重定向*/
        file[len] = '/';
        file[len + 1] = '\0';
        strcpy(host + strlen(host), file);
        sprintf(error_buf, "%s %s", host, "产生了301重定向");
        write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
        send_respond_head(bev, resp);
    } else {
        resp.no = 200;
        strcpy(resp.desc, OK);
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
 * 处理http请求的核心函数
 * @param bev
 */
void handle_http_request (struct bufferevent *bev) {
    map_str_t request_head;/**用于存储已经经过解析的http请求头*/
    map_init(&request_head);
    response_struct resp;
    init_resp(&resp);
    char request[4096] = {0};
    char error_buf[128] = {0};
    bufferevent_read(bev, request, sizeof(request));
    char page[512];
    char method[12], path[1024], protocol[12], *host;
    sscanf(request, "%[^ ] %[^ ] %[^ \r\n] ", method, path, protocol);
    /**
     * 获取请求头中的 Host:
     */
    host = get_headval(&request_head, request, "Host");
    if (!host) {
        write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "请求头解析错误");
        fprintf(stderr, "请求头解析错误\n");
        fflush(stderr);
        exit(1);
    }
    char *cookie = get_headval(&request_head, request, "Cookie");
    /**为了应对chrome自带的cookie*/
    int is_set_cookie = 0;
    char cookies[128] = {0};
    char *u = NULL;
    if (cookie) {
        /*判断是否在之前设置过cookie中的uid值*/
        if (strncmp(cookie, "uid", 3) != 0) {
            u = parse_cookies(cookie, "uid");
            if (!u) {
                set_cookie(cookies);
                is_set_cookie = 1;
            } else {
                /*使用原来有的cookie*/
            }
        }
    }
    if (is_set_cookie) {
        strcpy(resp.cookie, cookies);
        if (u != NULL)
            free(u);
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
                char *host_;
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
                strcpy(resp.type, "text/html; charset=utf-8");
                send_respond_head(bev, resp);
            }
        }
        if (!is_bloading) {
            struct stat st;
            int ret = stat(file, &st);
            if (ret == -1) {
                strcpy(page, file);
                sprintf(file, p->static_path, page);
                /**判断当前访问的文件是否位于static目录下(一般用于重要的静态页面)*/
                if (!stat(file, &st)) {
                    strcpy(resp.desc, OK);
                    resp.no = 200;
                    resp.len = st.st_size;
                    strcpy(resp.type, get_file_type(file));
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
                resp.len = st.st_size;
                strcpy(resp.type, get_file_type(file));
                send_filedir(bev, file, host, st, resp);
            }
        }
    } while (0);
    /**释放动态申请的资源*/
    const char *key;
    map_iter_t iter = map_iter(&m);
    while ((key = map_next(&request_head, &iter))) {
        char *val_d = *map_get(&request_head, key);
        free(val_d);
    }
    map_deinit(&request_head);
}

/**
 * 当libevent产生读事件后服务器自定义的请求处理函数
 * @param bev
 * @param arg : no use
 */
void read_cb (struct bufferevent *bev, void *arg) {
    handle_http_request(bev);
}


void write_cb (struct bufferevent *bev, void *arg) {
    fflush(stderr);
}

void event_cb (struct bufferevent *bev, short events, void *arg) {
    if (events & BEV_EVENT_EOF) {
        //todo
    } else if (events & BEV_EVENT_ERROR) {
        //todo
    }
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
void listener_init (
        struct evconnlistener *listener,
        evutil_socket_t fd,
        struct sockaddr *addr,
        int len, void *ptr) {
    struct event_base *base = (struct event_base *) ptr;
    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (bev == NULL) {
        fprintf(stderr, "创建监听失败\n");
        fflush(stderr);
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "创建监听失败，server退出");
        exit(1);
    } else {
        char buf[128];
        sprintf(buf, "创建监听成功,服务器套接字为:%d", fd);
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, buf);
    }
    evutil_make_socket_nonblocking(fd);
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_READ);
}

server_config_package *init_server_config (const char *json_path) {
    server_config_package *package = parse_json(json_path);
    if (package == NULL) {
        write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "配置文件初始化失败");
        return NULL;
    } else {
        /*创建线程池(最小线程个数，最大线程个数，最大工作队列长度) */
        package->pool = threadpool_create(5, 40, 40);
        return package;
    }
}


void event_init_listener (struct evconnlistener *listener, struct event_base *base, struct sockaddr_in serv) {

    listener = evconnlistener_new_bind(base, listener_init, base,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                       36, (struct sockaddr *) &serv, sizeof(serv));
    if (listener != NULL) {
        write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "绑定服务器端口成功");
    } else {
        /**
         * 启动配置文件中选定的备用端口
         */
        serv.sin_port = htons(p->al_port);
        listener = evconnlistener_new_bind(base, listener_init, base,
                                           LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                           36, (struct sockaddr *) &serv, sizeof(serv));
        if (listener != NULL) {
            write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "备用服务器端口启动成功");
        } else {
            fprintf(stderr, "备用服务器端口启动失败\n");
            fflush(stderr);
            write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "备用服务器端口启动失败");
            exit(1);
        }
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);
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

static void
accept_error_cb (struct evconnlistener *listener, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    char error_buf[128];
    /**当服务器因为超量的请求而产生宕机时候，对服务器进行重启*/
    sprintf(error_buf, "%d (%s) on the listener. "
                       "Shutting down.\n", err, evutil_socket_error_to_string(err));
    write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, error_buf);
    do {
        /**释放原先服务器的资源,并尝试重启*/
        event_base_loopexit(base, NULL);
        evconnlistener_free(listener);
        event_base_free(base);
        write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, "服务器重启");
        socket_serv_process();
    } while (0);

}

void socket_serv_process () {
    struct sockaddr_in serv;
    evthread_use_pthreads();
    struct event_base *base;
    base = event_base_new();
    struct evconnlistener *listener;
    serv = init_serv();
    event_init_listener(listener, base, serv);
    /*释放libevent资源*/
    evconnlistener_free(listener);
    event_base_free(base);
}
void server_init (const char *json_path) {
    /**根据json格式的配置初始化服务器配置*/
    p = init_server_config(json_path);
    if (!p) {
        fprintf(stderr, "初始化Json配置失败\n");
        fflush(stderr);
        exit(1);
    }
    /**初始化log文件*/
    if (open_log_fd(p->pool, p->log_path) != 0) {
        fprintf(stderr, "打开log文件失败\n");
        fflush(stderr);
        exit(1);
    }
    write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "初始化服务器成功");
    int ret = chdir(p->map_path);
    if (ret) {
        fprintf(stderr, "切换工作目录失败\n");
        fflush(stderr);
        write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "切换工作目录失败");
        exit(1);
    }
    socket_serv_process();
    exit(0);
}

