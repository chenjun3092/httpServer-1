#include "redis_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "post/redis_pool.h"


/**redis连接池*/
redisConnectionPool *redis_pool = NULL;


/**
 * 更新司机的位置
 * @param cla  司机的经度
 * @param clo 司机的纬度
 * @param session_id 司机的session id
 */
void upload_driver_location (char *cla, char *clo, char *session_id) {
    //GEOADD Driver 120.1676300000   29.6934100000  "Datangs"
    if (!redis_pool) {
        struct timeval timeout = {1, 500000};
        redis_pool = redisCreateConnectionPool(5, rp.host, rp.port, timeout);
    }
    redisContext *c = redisGetConnectionFromConnectionPool(redis_pool);
    if (c->err) {
        redisFree(c);
        return;
    }
    double la = atof(cla);
    double lo = atof(clo);
    char *command1 = (char *) malloc(sizeof(char) * 128);

    memset(command1, '\0', 128);
    sprintf(command1, "zrem Driver %s", session_id);
    redisReply *r1 = (redisReply *) redisCommand(c, command1);

    memset(command1, '\0', 128);
    /**司机将自身的位置放入到ready池中*/
    sprintf(command1, "GEOADD Driver %lf %lf %s", la, lo, session_id);
    redisReply *r2 = (redisReply *) redisCommand(c, command1);


    redisPutConnectionInConnectionPool(c, redis_pool);
    freeReplyObject(r1);
    freeReplyObject(r2);
    free(command1);
}

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
    /**交还数据库连接*/
    redisPutConnectionInConnectionPool(c, redis_pool);
    freeReplyObject(r);
    free(command1);
    return locations;
}

