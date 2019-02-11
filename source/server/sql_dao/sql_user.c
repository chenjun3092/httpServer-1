#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>
#include "user_pojo.h"

MYSQL *create_conn () {
    MYSQL *conn = NULL;
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "localhost", "root", "", "tc", 0, NULL, 0)) {
        return NULL;
    }
    return conn;
}

int insert_user_todb (struct user *u) {
    MYSQL *conn = create_conn();
    if (conn == NULL) {
        return 1;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    char *query = "insert into tc_user (u_id,u_name,password,phone,createtime,email,id_card,driver) values(?,?,?,?,CURRENT_TIMESTAMP(),?,?,?);";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        return 1;
    }
    MYSQL_BIND params[7];
    memset(params, 0, sizeof(params));

    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &u->id;
    params[0].buffer_length = sizeof(u->id);

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = u->username;
    params[1].buffer_length = strlen(u->username);

    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = u->paswd;
    params[2].buffer_length = strlen(u->paswd);

    params[3].buffer_type = MYSQL_TYPE_STRING;
    params[3].buffer = u->phone;
    params[3].buffer_length = strlen(u->phone);

    params[4].buffer_type = MYSQL_TYPE_STRING;
    params[4].buffer = u->email;
    params[4].buffer_length = strlen(u->email);

    params[5].buffer_type = MYSQL_TYPE_STRING;
    params[5].buffer = u->id_card;
    params[5].buffer_length = strlen(u->id_card);

    params[6].buffer_type = MYSQL_TYPE_STRING;
    params[6].buffer = u->is_driver;
    params[6].buffer_length = strlen(u->is_driver);

    mysql_stmt_bind_param(stmt, params);
    int res = mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);
    mysql_close(conn);
    if (res) {
        return 1;
    } else {
        return 0;
    }
}

int compare_user_indb (char *username, char *password) {
    MYSQL *conn = create_conn();
    if (conn == NULL) {
        return 1;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(conn); //创建MYSQL_STMT句柄
    char *query = "select * from tc_user where u_name=? and password = ?;";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        return 1;
    }
    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = username;
    params[0].buffer_length = strlen(username);

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = password;
    params[1].buffer_length = strlen(password);

    mysql_stmt_bind_param(stmt, params);
    mysql_stmt_execute(stmt);
    mysql_stmt_store_result(stmt);
    int res = 0;
    while (!mysql_stmt_fetch(stmt)) {
        res++;
    }
    mysql_stmt_close(stmt);
    mysql_close(conn);
    if (res) {
        return 0;
    } else {
        return 1;
    }
}