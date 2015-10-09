#ifndef SQL_CH_H
#define SQL_CH_H

#define SQLV_SUCCESS 0
#define SQLV_FAILURE 1
#define SQLV_USER_NOT_FOUND 2
#define SQLV_WRONG_PASSWORD 3
#define SQLV_CONNECTION_ERROR 4
#define SQLV_USER_EXISTS 5

void sql_ch_init();

int sql_check_user_auth(char *user, char *pw, int *res);

int sql_ch_add_user(char *user, char *pw);

void sql_ch_destroy();

#endif
