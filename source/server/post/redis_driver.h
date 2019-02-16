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

struct driver_locations *get_driver_locations (char *la, char *lo, long length);

#endif //HTTPD_REDIS_DRIVERLOCATION_H
