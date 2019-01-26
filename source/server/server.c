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
#include <openssl/ssl.h>
#include <openssl/err.h>

/*服务器全局配置*/
server_config_package *p = NULL;

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


void send_respond_head (struct bufferevent *bev, int no, const char *desc, const char *type, long len, char *host) {
    char buf[1024] = {0};
    /**状态行*/
    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, desc);
    /**消息报头*/
    sprintf(buf + strlen(buf), "Content-Type:%s\r\n", type);
    /**消息的长度*/
    sprintf(buf + strlen(buf), "Content-Length:%ld\r\n", len);

    if (no == 301) {
        sprintf(buf + strlen(buf), "Location:http://%s\r\n", host);
    } else if (no == 302) {
        /**用于被转发到tomcat的JavaWeb请求*/
        sprintf(buf + strlen(buf), "Location:%s\r\n", host);
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
void send_directory_ (struct bufferevent *bev, char *file, char *host) {
    size_t len = strlen(file);
    char error_buf[128] = {0};
    char t = file[len - 1];
    /**
     * 当确定请求的路径: http://server_path:port/dir 是一个目录时，补齐请求路径 .../dir 之后的 /
     * 并重定向到补齐之后的路径
     */
    if (t != '/') {
        /**通过http response code 产生重定向*/
        file[len] = '/';
        file[len + 1] = '\0';
        strcpy(host + strlen(host), file);
        sprintf(error_buf, "%s %s", host, "产生了301重定向");
        write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
        send_respond_head(bev, 301, "Moved Permanently", "text/html", -1, host);
    } else {
        send_respond_head(bev, 200, OK, "text/html; charset=utf-8", -1, NULL);
    }
    send_directory(bev, file);

}

/**
 * 当libevent产生读事件后服务器自定义的请求处理函数
 * @param bev
 * @param arg : no use
 */
void read_cb (struct bufferevent *bev, void *arg) {
    char request[4096] = {0};
    char error_buf[128] = {0};
    bufferevent_read(bev, request, sizeof(request));
    char page[512];
    char method[12], path[1024], protocol[12], host[256], no_use[8];
    sscanf(request, "%[^ ] %[^ ] %[^ \r\n] %[^ ] %[^ \r\n]", method, path, protocol, no_use, host);
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
                send_respond_head(bev, 302, "Moved Permanently", "text/html", -1, redirect_path);
            }
        }
    }
    do {
        int is_bloading = 0;
        int i;
        for (i = 0; i < p->d_len; ++i) {
            /**是否是负载均衡的路径*/
            if (!strcasecmp(path, p->dynamic_file[i])) {
                int n;
                char *host_;
                host_ = findiport(host, 0);
                *host_ = '\0';
                host_ = host;
                /*用随机的方法选择当前提供服务的服务器*/
                if (p->load_balancing) {
                    //实现一个简单的负载均衡
                    srand(time(NULL));
                    n = rand() % p->s_num;//负载均衡的种子
                }
                is_bloading = 1;
                /**将请求转发到负载均衡服务器*/
                sprintf(host, "%s:%d%s", host_, p->load_servers[n], path);
                sprintf(error_buf, "%s %s", host, "产生了301重定向");
                write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
                send_respond_head(bev, 301, "Moved Permanently", "text/html", -1, host);
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
                    if (S_ISDIR(st.st_mode)) {
                        send_directory_(bev, file, host);
                    } else if (S_ISREG(st.st_mode)) {
                        send_respond_head(bev, 200, OK, get_file_type(file), st.st_size, NULL);
                        send_file(bev, file);
                    }
                } else {
                    /**当前文件不存在且不是任何转发路径*/
                    memset(error_buf, '\0', strlen(error_buf));
                    sprintf(error_buf, "%s %s", file, "发生404错误");
                    write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, error_buf);
                    send_respond_head(bev, 404, "Not Found", "text/html", -1, NULL);
                    send_404file(bev);
                }
            } else {
                /*访问公开目录下的文件*/
                if (S_ISDIR(st.st_mode)) {
                    send_directory_(bev, file, host);
                } else if (S_ISREG(st.st_mode)) {
                    send_respond_head(bev, 200, OK, get_file_type(file), st.st_size, NULL);
                    send_file(bev, file);
                }
            }
        }
    } while (0);
}


void write_cb (struct bufferevent *bev, void *arg) {
    fflush(stdout);
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
        exit(1);
    }
    /**初始化log文件*/
    if (open_log_fd(p->pool, p->log_path) != 0) {
        exit(1);
    }
    write_log(INFO_L, getpid(), __FUNCTION__, __LINE__, "初始化服务器成功");
    int ret = chdir(p->map_path);
    if (ret) {
        write_log(EMERGE_L, getpid(), __FUNCTION__, __LINE__, "配置文件初始化失败");
        exit(1);
    }
    socket_serv_process();

    //fixme 退出程序的时候，线程池不写
    /**
     * 解决方案: 写父进程负责子进程退出时候的善后工作，但是因为这样不方便自己调试,
     * 故暂时不开发此功能
     */

    exit(0);
}

