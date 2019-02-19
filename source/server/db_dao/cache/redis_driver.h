//
// Created by fushenshen on 2019/2/16.
//

#ifndef HTTPD_REDIS_DRIVERLOCATION_H
#define HTTPD_REDIS_DRIVERLOCATION_H
#define  LEN 1024

#include <stdio.h>

struct driver_locations {
    char *str[1024];
    size_t len;
};

struct redis_server {
    char host[36];
    int port;
};

struct redis_server rp;

void upload_driver_location (char *cla, char *clo, char *addr_name);

struct driver_locations *get_driver_locations (char *la, char *lo, long length);

#endif //HTTPD_REDIS_DRIVERLOCATION_H
