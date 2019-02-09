#include "session.h"

/**
 *
 * 设置用户session对应的 {paramter,value} 映射
 * @param sessions 用户的session
 * @param paramter
 * @param value
 */
void set_parameter (session *sessions, char *paramter, char *value) {
    map_set(&sessions->parameters, paramter, value);
}

/**
 * 获取用户session paramter对应的值
 * @param sessions  用户的session
 * @param paramter
 * @return
 */
char *get_parameter (session *sessions, char *paramter) {
    map_str_t paramters = sessions->parameters;
    char **r = map_get(&paramters, paramter);
    if (!r) {
        return NULL;
    } else {
        return *r;
    }
}
