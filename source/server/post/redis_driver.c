#include "redis_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

/**
 * 查询司机的位置
 * @param la
 * @param lo
 * @param length
 * @return
 */
struct driver_locations *get_driver_locations (char *cla, char *clo, long length) {


    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c->err) {
        redisFree(c);
        return NULL;
    }
    double la = atof(cla);
    double lo = atof(clo);
    char *command1 = (char *) malloc(sizeof(char) * 128);
    memset(command1, '\0', 128);
    sprintf(command1, "GEORADIUS Driver %lf %lf  %ld m WITHDIST", la, lo, length);
    redisReply *r = (redisReply *) redisCommand(c, command1);
    struct driver_locations *locations = NULL;
    if (r->type == REDIS_REPLY_ARRAY) {
        size_t len = r->elements;
        locations = (struct driver_locations *) malloc(sizeof(struct driver_locations));
        locations->len = len;
        for (int i = 0; i < len; ++i) {
            char *res = ((r->element[i])->element[0])->str;
            locations->str[i] = (char *) malloc(strlen(res));
            strcpy(locations->str[i], res);
        }
    }
    freeReplyObject(r);
    free(command1);
    return locations;
}

