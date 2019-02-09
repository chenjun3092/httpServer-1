#include "post_handler.h"
#include <stdlib.h>
#include <server.h>
#include <memory.h>
#include "http_util.h"
#include "cJSON.h"


/**
 * 处理http post login请求的函数
 * @param arg 传入的参数
 * @return  返回处理的结果，一般为Json格式
 */
char *login (void *arg) {
    if (true) {
        const char *request = (char *) arg;
        char pay_str[1024] = {0};
        get_payload(request, pay_str);
        cJSON *response_root = cJSON_CreateObject();
        cJSON_AddStringToObject(response_root, "result", "ok");
        cJSON_AddStringToObject(response_root, "status", "登录成功");
        char *response_str = cJSON_Print(response_root);
        cJSON_Delete(response_root);
        return response_str;
    } else {
        return NULL;
    }
}

/**
 * 处理http post login请求的函数
 * @param arg 传入的参数
 * @return  返回处理的结果，一般为Json格式
 */
char *sign_in (void *arg) {
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

post_func post_func_array[] = {
        {NULL, NULL}};