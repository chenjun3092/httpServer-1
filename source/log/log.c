#include "log.h"
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include "mtime.h"
#include <stdlib.h>
#include <string.h>

threadpool_t *pool = NULL;
int fd = -1;

/**
 * 写日志的函数(由线程池中的工作线程调用)
 * @param buf
 * @return
 */
void *write_log_ (void *buf) {
    char *buf1 = (char *) buf;
    write(fd, buf, strlen(buf1));
    return NULL;
}
/**
 * 打开日志文件
 * @param p 线程池指针
 * @param log_path 日志文件的地址
 * @return
 */
int open_log_fd (threadpool_t *p, const char *log_path) {

    if (fd > 0) {
        return 0;
    } else {
        char path[128];
        sprintf(path, "%s.%d.%d", log_path, get_day(), get_hour());
        int try_time = 0;
        open_fd:
        fd = open(path, O_APPEND | O_CREAT | O_RDWR, 0666);
        if (fd < 0) {
            /*
             *  三次尝试打开Log文件
             **/
            try_time++;
            if (try_time >= 3) {
                return 1;
            }
            goto open_fd;
        }
        pool = p;
        return 0;
    }
}
/**
 * 根据具体的事件写不同等级的日志
 * @param level 日志的等级
 * @param pid 写日志的进程号
 * @param function 触发写日志的函数
 * @param line 写日志的位置
 * @param message 日志的具体消息
 */
void write_log (LOG_LEVEL level, pid_t pid, const char *function, int line, const char *message) {
    if (level < 1 || level > 5) {
        return;
    } else {
        char *buf = malloc(sizeof(char) * 1024);
        const char *times = get_datetime();
        switch (level) {
            case EMERGE_L: {
                sprintf(buf, "%s: %s %d function:%s line:%d %s \n", times, EMERGE, pid, function, line, message);
                break;
            }
            case ALERT_L: {
                sprintf(buf, "%s: %s %d function:%s line:%d %s\n", times, ALERT, pid, function, line, message);
                break;
            }
            case WARN_L: {
                sprintf(buf, "%s: %s %d function:%s line:%d %s\n", times, WARN, pid, function, line, message);
                break;
            }
            case INFO_L: {
                sprintf(buf, "%s: %s %d function:%s line:%d %s\n", times, INFO, pid, function, line, message);
                break;
            }
            case DEBUG_L: {
                sprintf(buf, "%s: %s %d function:%s line:%d %s\n", times, DEBUG, pid, function, line, message);
                break;
            }
            default://fixme 没有这个错误等级
                ;
        }
        if (pool != NULL) {
            threadpool_add(pool, write_log_, buf);
        }else {
            exit(1);
        }
        free((void *) times);
    }
}

