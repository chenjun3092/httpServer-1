#include "post_handler.h"
#include <stdlib.h>
#include <server.h>
#include <memory.h>
#include <init_config.h>
#include "http_util.h"
#include "cJSON.h"
#include "./http_component/session.h"


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
    struct session *s = *((struct session **) (map_get(&p->sessions, session_id)));
    cJSON *root = cJSON_Parse(pay_str);
    const char *username = NULL;
    const char *password = NULL;
    if (root != NULL) {
        cJSON *user = cJSON_GetObjectItem(root, "username");
        if (user)
            username = user->valuestring;
        cJSON *pswd = cJSON_GetObjectItem(root, "password");
        if (pswd)
            password = pswd->valuestring;
    }
    cJSON *response_root = cJSON_CreateObject();
    if (username && password) {
        char *tag = malloc(16);
        strcpy(tag, "online");
        map_set(&s->parameters, "status", tag);
        cJSON_AddStringToObject(response_root, "session_id", session_id);
        cJSON_AddStringToObject(response_root, "result", "ok");
        cJSON_AddStringToObject(response_root, "status", "登录成功");
    } else {
        if (!root) {
            cJSON_AddStringToObject(response_root, "result", "json格式错误");
        } else {
            cJSON_AddStringToObject(response_root, "result", "用户名或密码错误");
        }
        cJSON_AddStringToObject(response_root, "status", "登录失败");
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

post_func post_func_array[] = {{"/login",    post_login},
                               {"/register", post_sign_in},
                               {NULL, NULL}};