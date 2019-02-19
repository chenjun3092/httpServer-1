#include "post_handler.h"
#include <stdlib.h>
#include <server.h>
#include <memory.h>
#include <init_config.h>
#include "tool.h"
#include "http_util.h"
#include "cJSON.h"
#include "./http_component/session.h"
#include "db_dao/user_pojo.h"
#include "db_dao/sql_user.h"
#include "db_dao/cache/redis_driver.h"
#include <hiredis/hiredis.h>
#include <assert.h>

extern server_config_package *p;

/**
 * 处理http post login请求的函数
 * @param arg 传入的参数
 * @return  返回处理的结果，一般为Json格式
 */
char *post_login (void *arg, char *session_id) {
    const char *request = (char *) arg;
    char pay_str[1024] = {0};
    get_payload(request, pay_str);
    cJSON *response_root = cJSON_CreateObject();
    cJSON *root = cJSON_Parse(pay_str);
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    map_str_t params = s->parameters;
    if (map_get(&params, "status") != NULL) {
        cJSON_AddStringToObject(response_root, "result", "login already");
    } else {
        char *username = NULL;
        char *password = NULL;

        if (root != NULL) {
            cJSON *user = cJSON_GetObjectItem(root, "username");
            cJSON *pswd = cJSON_GetObjectItem(root, "password");
            if (user && pswd) {
                username = user->valuestring;
                password = malloc(sizeof(char) * 128);
                get_md5(pswd->valuestring, password);
                int res = compare_user_indb(username, password);
                if (!res) {
                    char *m = malloc(36);
                    strcpy(m, "idle");
                    map_set(&s->parameters, "status", m);
                    printf("session_id %s\n", session_id);
                    fflush(stdout);
                    cJSON_AddStringToObject(response_root, "result", "ok");
                    cJSON_AddStringToObject(response_root, "session_id", session_id);
                } else {
                    cJSON_AddStringToObject(response_root, "result", "failed");
                    cJSON_AddStringToObject(response_root, "reason", "用户名或密码错误");
                }
                free(password);
            } else {
                cJSON_AddStringToObject(response_root, "result", "failed");
                cJSON_AddStringToObject(response_root, "reason", "参数个数错误");
            }
        } else {
            cJSON_AddStringToObject(response_root, "result", "failed");
            cJSON_AddStringToObject(response_root, "reason", "json格式错误");
        }
    }

    char *response_str = cJSON_Print(response_root);
    cJSON_Delete(response_root);
    cJSON_Delete(root);
    return response_str;
}

/**
 * 处理http post login请求的函数
 * @param arg 传入的参数
 * @return  返回处理的结果，一般为Json格式
 */
char *post_sign_in (void *arg, char *session_id) {
    const char *request = (char *) arg;
    char pay_str[1024] = {0};
    get_payload(request, pay_str);
    cJSON *root = cJSON_Parse(pay_str);
    struct user u;
    cJSON *response_root = cJSON_CreateObject();
    if (root) {
        cJSON *username = cJSON_GetObjectItem(root, "username");
        cJSON *password = cJSON_GetObjectItem(root, "password");
        cJSON *phone = cJSON_GetObjectItem(root, "phone");
        cJSON *email = cJSON_GetObjectItem(root, "email");
        cJSON *id_card = cJSON_GetObjectItem(root, "id_card");
        cJSON *is_driver = cJSON_GetObjectItem(root, "is_driver");
        if (username && password && phone
            && email && id_card && is_driver) {
            u.id = rand_num();
            char *pswd = malloc(sizeof(char) * 128);
            get_md5(password->valuestring, pswd);
            u.paswd = pswd;
            u.email = email->valuestring;
            u.is_driver = is_driver->valuestring;
            u.phone = phone->valuestring;
            u.id_card = id_card->valuestring;
            u.username = username->valuestring;
            if (insert_user_todb(&u) != 0) {
                cJSON_AddStringToObject(response_root, "result", "failed");
                cJSON_AddStringToObject(response_root, "reason", "可能已存在同名的用户");
            } else {
                cJSON_AddStringToObject(response_root, "result", "ok");
                cJSON_AddStringToObject(response_root, "reason", "注册成功");
            }
            free(pswd);
        } else {
            /**Android前端一般已经产生过一次参数校验*/
            cJSON_AddStringToObject(response_root, "result", "failed");
            cJSON_AddStringToObject(response_root, "reason", "参数个数错误");
        }
    } else {
        cJSON_AddStringToObject(response_root, "result", "failed");
        cJSON_AddStringToObject(response_root, "reason", "json格式错误");
    }//root == NULL
    char *res = cJSON_Print(response_root);
    cJSON_Delete(response_root);
    cJSON_Delete(root);
    return res;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**乘客的逻辑*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * 更新乘客当前的位置,更新频率为2s 此时的状态为idle(与登录后保持一致,乘客还没有开始打车)
 * @param arg
 * @param session_id
 * @return
 */
char *passenger_idle (void *arg, char *session_id) {
    const char *request = (char *) arg;
    char pay_str[1024] = {0};
    get_payload(request, pay_str);
    cJSON *root = cJSON_Parse(pay_str);
    cJSON *latitude = cJSON_GetObjectItem(root, "latitude");
    cJSON *longitude = cJSON_GetObjectItem(root, "longitude");
    cJSON *address = cJSON_GetObjectItem(root, "address");
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    char **la_pre = map_get(&s->parameters, "latitude");
    char *la;
    if (la_pre) {
        free(*la_pre);
    }
    la = strdup(latitude->valuestring);
    map_set(&s->parameters, "latitude", la);

    char **lo_pre = map_get(&s->parameters, "longitude");
    char *lo;
    if (lo_pre) {
        free(*lo_pre);
    }
    lo = strdup(longitude->valuestring);
    map_set(&s->parameters, "longitude", lo);

    char **addr_pre = map_get(&s->parameters, "addr");
    char *addr;
    if (addr_pre) {
        free(*addr_pre);
    }
    addr = strdup(address->valuestring);
    map_set(&s->parameters, "addr", addr);

    cJSON *response_root = cJSON_CreateObject();
    /**在redis中查询周围的司机*/
    struct driver_locations *locations;
    /**显示附近距离为10km的司机*/
    locations = get_driver_locations(la, lo, 10000);
    if (locations != NULL) {
        /**获取司机位置成功*/
        cJSON_AddStringToObject(response_root, "result", "ok");
        cJSON *drivers = cJSON_CreateArray();
        cJSON_AddItemToObject(response_root, "drivers", drivers);
        /**在返回列表中添加司机的信息*/
        for (int i = 0; i < (int) locations->len; ++i) {
            cJSON *d = cJSON_CreateObject();
            cJSON_AddStringToObject(d, "driver", locations->str[i]);
            free(locations->str[i]);
            cJSON_AddItemToArray(drivers, d);
        }
        free(locations);
    } else {
        cJSON_AddStringToObject(response_root, "result", "failed");
    }
    char *res = cJSON_Print(response_root);
    /**返回周围位置的司机*/
    cJSON_Delete(response_root);
    cJSON_Delete(root);
    return res;
}

/**
 * 乘客开始打车选择最近的一个司机，乘客进入waiting状态
 * @param arg
 * @param session_id
 * @return
 */
void *passenger_take_car (void *arg, char *session_id) {
    const char *request = (char *) arg;
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    map_str_t params = s->parameters;
    cJSON *response_root = cJSON_CreateObject();
    char **str = map_get(&params, "status");
    char pay_str[1024] = {0};
    /**解析http数据中post请求信息*/
    get_payload(request, pay_str);
    cJSON *root = cJSON_Parse(pay_str);
    struct driver_locations *locations;
    if (str) {
        char *la = *map_get(&params, "latitude");
        char *lo = *map_get(&params, "longitude");
        /**打车要去的地方*/
        cJSON *latitude_to = cJSON_GetObjectItem(root, "latitude");
        cJSON *longitude_to = cJSON_GetObjectItem(root, "longitude");
        cJSON *address_to = cJSON_GetObjectItem(root, "address");
        char *la_to = strdup(latitude_to->valuestring);
        char *lo_to = strdup(longitude_to->valuestring);
        char *addr_to = strdup(address_to->valuestring);
        /**标记乘客打车地点目标的位置*/
        map_set(&s->parameters, "latitude_to", la_to);
        map_set(&s->parameters, "longitude_to", lo_to);
        map_set(&s->parameters, "addr_to", addr_to);

        int radius = 5000;/**开始搜索司机的范围*/
        while (true) {
            locations = get_driver_locations(la, lo, radius);
            if (locations != NULL) {
                if (locations->len >= 1) {/**在radius之内有司机可以派单*/
                    /**获取司机位置成功*/
                    cJSON_AddStringToObject(response_root, "result", "ok");
                    for (int i = 0; i < (int) locations->len; ++i) {
                        if (!i) {
                            /**设置乘客的状态为waiting:即已经被司机接单*/
                            char *status = *str;
                            memset(status, '\0', strlen(status));
                            /**乘客进入waiting状态,即等待被接单以及等待接车的过程*/
                            strcpy(status, "waiting");


                            /**返回司机的session id*/
                            cJSON_AddStringToObject(response_root, "driver", locations->str[i]);
                            char *d_session_id = malloc(sizeof(char) * strlen(locations->str[i]));
                            strcpy(d_session_id, locations->str[i]);
                            /**在session中保存司机的session_id*/
                            map_set(&s->parameters, "driver_session", d_session_id);
                            //fixme 需要将司机(driver_session)的状态设置为 taking
                            struct session *driver_session = *((struct session **) (map_get(&p->sessions,
                                                                                            d_session_id)));
                            str = map_get(&driver_session->parameters, "status");
                            status = *str;
                            /**更改司机当前的状态以及记录当前的目的地和乘客的session_id*/
                            strcpy(status, "taking");//表示当前司机的动作是前去接乘客
                            map_set(&driver_session->parameters, "latitude_to", strdup(latitude_to->valuestring));
                            map_set(&driver_session->parameters, "longitude_to", strdup(longitude_to->valuestring));
                            map_set(&driver_session->parameters, "addr_to", strdup(address_to->valuestring));
                            map_set(&driver_session->parameters, "p_session_id", strdup(session_id));
                        }
                        free(locations->str[i]);
                    }
                    free(locations);
                    break;
                } else {
                    radius += 1000;
                    if (radius >= 11000) {
                        /**在十公里内没有司机可以派单*/
                        break;
                    }
                }
            } else {
                cJSON_AddStringToObject(response_root,
                                        "result", "failed");
                break;
            }
        }
    } else {
        cJSON_AddStringToObject(response_root,
                                "result", "failed");
        cJSON_AddStringToObject(response_root,
                                "reason", "不明原因");
    }

    char *res = cJSON_Print(response_root);
    cJSON_Delete(response_root);
    return res;
}

/**
 * 司机已经将乘客接上车,在乘客端进行操作
 * @param arg
 * @param session_id
 * @return
 */
void *passenger_catched (void *arg, char *session_id) {
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    cJSON *response_root = cJSON_CreateObject();
    cJSON_AddStringToObject(response_root, "status", "ok");
    char *res = cJSON_Print(response_root);
    map_str_t params = s->parameters;
    char **str = map_get(&params, "status");
    char *status = *str;
    cJSON_Delete(response_root);
    return res;
}

/**
 * 乘客到达目的地后 由司机调用
 * @param arg
 * @param session_id
 * @return
 */
void *passenger_get_point (void *arg, char *session_id) {
    /**乘客达到目的地之后的 确认应该由司机发出*/
    /**在session中删除此次打车的信息*/
    char *res;
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "ok");
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    map_str_t params = s->parameters;
    char **str = map_get(&params, "status");
    char *status = *str;
    res = cJSON_Print(resp);
    cJSON_Delete(resp);
    return res;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**司机的逻辑*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * 每隔0.5s更新一次司机的当前位置,状态仍然保持为idle
 * @param arg
 * @param session_id
 * @return
 */
void *driver_idle (void *arg, char *session_id) {
    const char *request = (char *) arg;
    char pay_str[1024] = {0};
    get_payload(request, pay_str);
    cJSON *root = cJSON_Parse(pay_str);
    cJSON *latitude = cJSON_GetObjectItem(root, "latitude");
    cJSON *longitude = cJSON_GetObjectItem(root, "longitude");
    cJSON *address = cJSON_GetObjectItem(root, "address");
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));

    /**更新司机的当前位置*/
    char **la_pre = map_get(&s->parameters, "latitude");
    char *la;
    if (la_pre) {
        free(*la_pre);
    }
    la = strdup(latitude->valuestring);
    map_set(&s->parameters, "latitude", la);

    char **lo_pre = map_get(&s->parameters, "longitude");
    char *lo;
    if (lo_pre) {
        free(*lo_pre);
    }
    lo = strdup(longitude->valuestring);
    map_set(&s->parameters, "longitude", lo);

    char **addr_pre = map_get(&s->parameters, "addr");
    char *addr;
    if (addr_pre) {
        free(*addr_pre);
    }
    addr = strdup(address->valuestring);
    map_set(&s->parameters, "addr", addr);

    /**构造响应头*/
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "result", "ok");
    char *res = cJSON_Print(resp);
    cJSON_Delete(resp);
    cJSON_Delete(root);
    return res;
}

/**
 * 司机准备接单,状态更新为ready
 * @param arg
 * @param session_id
 * @return
 */
void *driver_ready (void *arg, char *session_id) {
    const char *request = (char *) arg;
    cJSON *resp = cJSON_CreateObject();

    char pay_str[1024] = {0};
    get_payload(request, pay_str);
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    char **str = map_get(&s->parameters, "status");
    if (str) {
        char *status = *str;
        memset(status, '\0', strlen(status));
        /**司机的状态进入ready ,准备接单的状态*/
        strcpy(status, "ready");
        /**得到最多在0.5s前更新的位置*/
        char *la = *map_get(&s->parameters, "latitude");
        char *lo = *map_get(&s->parameters, "longitude");
        /**上传自身的位置*/
        upload_driver_location(la, lo, session_id);
        /*当接到单子之后状态改为catching*/
    } else {
        /**一般情况为司机的状态不对*/
        cJSON_AddStringToObject(resp, "result", "failed");
    }
    char *res = cJSON_Print(resp);
    cJSON_Delete(resp);
    return res;
}

/**
 * 司机已经在接到乘客后进入驾驶状态
 * @param arg
 * @param session_id
 * @return
 */
void *driver_catched (void *arg, char *session_id) {
    cJSON *resp = cJSON_CreateObject();
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    map_str_t params = s->parameters;
    char *addr_to = *map_get(&params, "addr_to");
    char *p_id = *map_get(&params, "p_session_id");

    struct session *passenger_session = *((struct session **) (map_get(&p->sessions, p_id)));
    map_str_t passenger_params = passenger_session->parameters;
    //修改乘客的状态
    char **str = map_get(&passenger_params, "status");
    char *status = *str;
    strcpy(status, "traveling");

    cJSON_AddStringToObject(resp, "乘客目的地", addr_to);
    cJSON_AddStringToObject(resp, "乘客id", p_id);

    /**修改司机的状态*/
    str = map_get(&params, "status");
    status = *str;
    strcpy(status, "driving");

    char *res = cJSON_Print(resp);
    cJSON_Delete(resp);
    return res;
}

// 乘客到达后，司机需要手动设置乘客的下车状态,同时清除自身以及对应乘客对于该订单的信息

/**
 *
 * @param arg
 * @param session_id
 * @return
 */
void *driver_get_point (void *arg, char *session_id) {
    /**清除司机自身关于该订单的信息*/
    cJSON *resp = cJSON_CreateObject();
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    char *p_id = *map_get(&s->parameters, "p_session_id");
    for (int i = 0; i < 2; ++i) {
        char **str = map_get(&s->parameters, "status");
        char *status = *str;
        memset(status, '\0', strlen(status));
        strcpy(status, "idle");
        const char *key_ = NULL;
        map_iter_t iter = map_iter(&s->parameters);
        while ((key_ = map_next(&s->parameters, &iter))) {
            if (!strcmp(key_, "status")) {
                continue;
            }
            char *v = *map_get(&s->parameters, key_);
            if (v != NULL) {
                free(v);
            }
            map_remove(&s->parameters, key_);
        }
        /**清除乘客关于该订单的信息*/
        s = *((struct session **) (map_get(&p->sessions, p_id)));
    }
    cJSON_AddStringToObject(resp, "status", "ok");
    char *res = cJSON_Print(resp);
    cJSON_Delete(resp);

    //todo 关于记录账单的信息
    return res;
}


post_func post_func_array[] = {{"/login",     post_login},
                               {"/register",  post_sign_in},
                               {"/p_idle",    passenger_idle},
                               {"/p_take",    passenger_take_car},
                               {"/p_catched", passenger_catched},
                               {"/p_get",     passenger_get_point},
                               {"/d_idle",    driver_idle},
                               {"/d_ready",   driver_ready},
                               {"/d_catched", driver_catched},
                               {"/d_get",     driver_get_point},
                               {NULL, NULL}};