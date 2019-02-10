#include "mtime.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>


long  get_unix_timestamp(){
    time_t t;
    t = time(NULL);
    return (long)time(&t);
}
/**
 *
 * @return cookie所使用的过期时间格式
 */
char *get_cookie_time () {
    char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    char *wmon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    time_t timep;
    struct tm *p;
    char *t = malloc(sizeof(char) * 64);
    time(&timep); /*获得time_t结构的时间，UTC时间*/
    p = localtime(&timep); /*转换为struct tm结构的当地时间*/
    /** Expires=Wed, 21 Oct 9999 12:59:59 GMT;*/
    sprintf(t, "%s, %d %s %04d %02d:%02d:%02d CST", wday[p->tm_wday], p->tm_mday, wmon[p->tm_mon], p->tm_year + 1900,
            p->tm_hour,
            p->tm_min,
            p->tm_sec);
    return t;
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


/**
 * @return 返回系统时间中的天
 */
int get_day () {
    time_t t = time(NULL);
    struct tm tm_struct = *localtime(&t);
    int day = tm_struct.tm_yday;
    return day + 1;
}
