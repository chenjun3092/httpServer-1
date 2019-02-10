#include "get_handler.h"
#include <stdlib.h>
#include <server.h>
#include <init_config.h>
#include "cJSON.h"
#include "./http_component/session.h"

extern server_config_package *p;


/**
 * 处理http post login请求的函数
 * @param arg 传入的参数
 * @return  返回处理的结果，一般为Json格式
 */
char *get_login (char *session_id, void *arg) {
    char *request = (char *) arg;
    char *sid = (char *) session_id;
    struct session *s = *((struct session **) (map_get(&p->sessions, sid)));
    char *u = get_parameter(s, "username");
    char *p = get_parameter(s, "password");
    char *res;
    int is_succ = 1;
    if (!u || !p) {
        res = "用户名和密码不能为空";
        is_succ = 0;
    } else {
        res = "登录成功";
    }
    cJSON *response_root = cJSON_CreateObject();
    if (is_succ) {
        cJSON_AddStringToObject(response_root, "session_id", sid);
        cJSON_AddStringToObject(response_root, "result", "success");
    } else {
        cJSON_AddStringToObject(response_root, "result", "failed");
    }
    cJSON_AddStringToObject(response_root, "status", res);
    char *response_str = cJSON_Print(response_root);
    cJSON_Delete(response_root);

    return response_str;

}

/**
 * 处理http post login请求的函数
 * @param arg 传入的参数
 * @return  返回处理的结果，一般为Json格式
 */
char *get_sign_in (char *session_id, void *arg) {
    if (true) {
        cJSON *response_root = cJSON_CreateObject();
        cJSON_AddStringToObject(response_root, "result", "ok");
        char *response_str = cJSON_Print(response_root);
        cJSON_Delete(response_root);
        return response_str;
    } else {
        return NULL;
    }
}

get_function get_func_array[] = {{"/login", get_login},
                                 {"/reg",   get_sign_in},
                                 {NULL, NULL}};