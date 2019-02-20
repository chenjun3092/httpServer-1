#ifndef HTTPD_SQL_INSERT_H
#define HTTPD_SQL_INSERT_H

#include "user_pojo.h"

int insert_user_todb (struct user *u);

int compare_user_indb (char *username, char *passwd);

char *query_is_driver (char *u_name);

#endif //HTTPD_SQL_INSERT_H
