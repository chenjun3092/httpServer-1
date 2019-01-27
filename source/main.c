#include <stdlib.h>
#include <init_config.h>
#include <log.h>
#include "server.h"

const char *jpath;
extern server_config_package *p;
#include <stdlib.h>
#include <zconf.h>
#include <init_config.h>
#include <log.h>
#include "server.h"


void do_restart (int signo){
    pid_t pid;
    pid = fork();
    if(!pid){
        fprintf(stderr, "服务器重启");
        fflush(stdout);
        server_init(jpath);
    }else {
        /*parent process*/
        signal(SIGCHLD, do_restart);
        wait(NULL);
    }
}

#if 1
int main (int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "ex: httpserv json_path");
        exit(1);
    }
    jpath = argv[1];
    server_init(jpath);
}

#endif


#if 0

int main (int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "ex: httpserv json_path");
        exit(1);
    }
    jpath = argv[1];
    pid_t pid;
    if ((pid = fork()) < 0) {
        fprintf(stderr, "http服务器初始化失败");
        fflush(stdout);
    } else if (pid > 0) {
        fprintf(stdout,"watch process pid:%d\n",getpid());
        fflush(stdout);
        signal(SIGCHLD, do_restart);
        wait(NULL);
    } else {
        fprintf(stdout,"worker process pid:%d\n",getpid());
        fflush(stdout);
        server_init(jpath);
    }
    return 0;
}

#endif
