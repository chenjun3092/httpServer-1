#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include <stdlib.h>
#include "parse_json.h"
#include "log.h"

server_config_package *parse_json (const char *json_path) {
    // 加载json文件
    int i;
    FILE *fp = fopen(json_path, "r");
    if (fp == NULL) {
        return NULL;
    }
    char *buf = malloc(4096 * sizeof(char));
    memset(buf, '\0', 4096);
    fread(buf, 1, 4096, fp);
    server_config_package *p = malloc(sizeof(server_config_package));
    cJSON *root = cJSON_Parse(buf);
    cJSON *tmp = root;

    cJSON *subobj = cJSON_GetObjectItem(root, "server");
    // 判断对象是否存在
    p->site_num = p->d_len = p->s_num = p->r_num = 0;
    if (subobj) {
        // 获取子对象
        cJSON *port = cJSON_GetObjectItem(subobj, "port");
        if (port) {
            p->listen_port = port->valueint;
        }
        cJSON *error_page = cJSON_GetObjectItem(subobj, "error_page");
        if (error_page) {
            p->error_page = error_page->valuestring;
        } else {
            /*必须配置错误页面*/
            fprintf(stderr, "错误页面没有设置\n");
            fflush(stderr);
            write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, "错误页面配置失败");
            exit(1);
        }


        cJSON *map_path = cJSON_GetObjectItem(subobj, "map_path");
        if (map_path) {
            p->map_path = map_path->valuestring;
        }

        cJSON *index = cJSON_GetObjectItem(subobj, "index");
        if (index) {
            p->index_html = index->valuestring;
        }


        cJSON *redirect_path = cJSON_GetObjectItem(subobj, "redirect_path");
        if (redirect_path != NULL) {
            if (redirect_path->type == cJSON_Array) {
                for (i = 0; i < cJSON_GetArraySize(redirect_path); ++i) {
                    cJSON *d_node = cJSON_GetArrayItem(redirect_path, i);
                    p->redirect_path[i] = d_node->valuestring;
                    p->r_num++;
                }
            }
        } else {
            p->redirect_path[0] = NULL;
        }

        cJSON *log_path = cJSON_GetObjectItem(subobj, "log_path");
        if (log_path) {
            p->log_path = log_path->valuestring;
        }

        cJSON *alternate_port = cJSON_GetObjectItem(subobj, "alternate_port");
        if (alternate_port) {
            p->al_port = alternate_port->valueint;
        }

        cJSON *static_page = cJSON_GetObjectItem(subobj, "static_page");
        if (static_page) {
            p->static_path = static_page->valuestring;
        }

        cJSON *load_balancing = cJSON_GetObjectItem(subobj, "load_balancing");
        if (load_balancing != NULL) {
            p->load_balancing = load_balancing->valueint;
        }


        cJSON *dynamic_file = cJSON_GetObjectItem(subobj, "dynamic_file");
        if (dynamic_file != NULL) {
            if (dynamic_file->type == cJSON_Array) {
                for (i = 0; i < cJSON_GetArraySize(dynamic_file); ++i) {
                    cJSON *d_node = cJSON_GetArrayItem(dynamic_file, i);
                    p->dynamic_file[i] = d_node->valuestring;
                    p->d_len++;
                }
            }
        }
        cJSON *load_severs = cJSON_GetObjectItem(subobj, "load_servers");
        if (load_severs) {
            if (dynamic_file->type == cJSON_Array) {
                for (i = 0; i < cJSON_GetArraySize(load_severs); ++i) {
                    cJSON *d_node = cJSON_GetArrayItem(load_severs, i);
                    p->load_servers[i] = d_node->valueint;
                    p->s_num++;
                }
            }
        }

        cJSON *redirect_site = cJSON_GetObjectItem(subobj, "redirect_site");
        if (redirect_site) {
            if (redirect_site->type == cJSON_Array) {
                for (i = 0; i < cJSON_GetArraySize(redirect_site); ++i) {
                    cJSON *d_node = cJSON_GetArrayItem(redirect_site, i);
                    p->redirect_site[i] = d_node->valuestring;
                    p->site_num++;
                }
            }
        } else {
            p->redirect_site[0] = NULL;
        }
        if (p->site_num != p->r_num) {
            fprintf(stderr, "转发规则设置错误\n");
            fflush(stderr);
            write_log(WARN_L, getpid(), __FUNCTION__, __LINE__, "转发规则与转发的网站数目不同");
            exit(1);
        }
    }
    free(buf);
    fclose(fp);

    return p;
}
