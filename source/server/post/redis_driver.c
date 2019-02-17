#include "redis_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "redispool.h"


/**redis连接池*/
redisConnectionPool *redis_pool = NULL;


/**
 * 查询司机的位置
 * @param la
 * @param lo
 * @param length
 * @return
 */

struct driver_locations *get_driver_locations (char *cla, char *clo, long length) {
    if (!redis_pool) {
        struct timeval timeout = {1, 500000};
        redis_pool = redisCreateConnectionPool(5, rp.host, rp.port, timeout);
    }
    redisContext *c = redisGetConnectionFromConnectionPool(redis_pool);
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
    redisFree(c);
    return locations;
}

