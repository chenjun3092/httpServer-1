#ifndef SERVER_H
#define SERVER_H
#define DIR_PATH "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>"
#define DIR_NAME "<html><head><title>目录名: %s</title></head>"
#define REG_PATH "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>"
#define DIR_CUR_NAME "<body><h1>当前目录: %s</h1><table>"
#define OK "OK"
#define true 1

#include <event2/listener.h>
#include <event2/bufferevent.h>

typedef struct response_struct {
    int no;
    long len;
    const char *host;
    const char *desc;
    const char *type;
    const char *cookie;
    /* const char *date;
     const char *last_modified;*/
} response_struct;
#define END_TABLE "</table></body></html>"

void send_404file (struct bufferevent *bev);

void send_500file (struct bufferevent *bev);

void send_file (struct bufferevent *bev, const char *filename);

void send_respond_head (struct bufferevent *bev, struct response_struct resp);

void send_directory (struct bufferevent *bev, const char *dirname);

void send_directory_ (struct bufferevent *bev, char *file, char *host,struct response_struct resp);

void read_cb (struct bufferevent *bev, void *arg);

void write_cb (struct bufferevent *bev, void *arg);

void event_cb (struct bufferevent *bev, short events, void *arg);

void socket_serv_process ();

struct sockaddr_in init_serv ();

void listener_init (
        struct evconnlistener *listener,
        evutil_socket_t fd,
        struct sockaddr *addr,
        int len, void *ptr);

void server_init (const char *);

static void
accept_error_cb (struct evconnlistener *listener, void *ctx);

void event_init_listener (struct evconnlistener *listener, struct event_base *base, struct sockaddr_in serv);


#endif // SERVER_H
