#include "post_handler.h"
#include <stdlib.h>
#include <server.h>
#include <memory.h>
#include <init_config.h>
#include "tool.h"
#include "http_util.h"
#include "cJSON.h"
#include "./http_component/session.h"
#include "sql_dao/user_pojo.h"
#include "sql_dao/sql_user.h"

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
                password = pswd->valuestring;
                int res = compare_user_indb(username, password);
                if (!res) {
                    char *m = malloc(8);
                    strcpy(m, "online");
                    map_set(&s->parameters, "status", m);
                    cJSON_AddStringToObject(response_root, "result", "ok");
                    cJSON_AddStringToObject(response_root, "session_id", session_id);
                } else {
                    cJSON_AddStringToObject(response_root, "result", "failed");
                    cJSON_AddStringToObject(response_root, "reason", "用户名或密码错误");
                }
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
            u.paswd = password->valuestring;
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

post_func post_func_array[] = {{"/login",    post_login},
                               {"/register", post_sign_in},
                               {NULL, NULL}};