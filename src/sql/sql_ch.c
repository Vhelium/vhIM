#include <stdlib.h>
#include <stdio.h>
#include <mysql.h>
#include <string.h>

#include "sql_ch.h"
#include "../constants.h"

struct sql_conection {
    char server[64];
    char user[64];
    char pw[64];
    char db[64];
} sql_con;

static char query[256];

void sql_ch_init()
{
    strcpy(sql_con.server, "localhost");
    strcpy(sql_con.user, "vhIM");
    strcpy(sql_con.pw, "vhInMa");
    strcpy(sql_con.db, "vhIM");
}

int sql_check_user_auth(char *user, char *pw)
{
    int r = SQLV_FAILURE;
    MYSQL *con;
    MYSQL_RES *res;
    MYSQL_ROW row;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        return SQLV_CONNECTION_ERROR;
    }

    //TODO: prevent slq injection via username
    sprintf(query, "SELECT `id`, `password` FROM `users` WHERE `user` = '%s' LIMIT 1",
            user);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        return SQLV_CONNECTION_ERROR;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        char *uid = row[0];
        char *upw = row[1];

        if (strcmp(pw, upw)) {
            r = SQLV_WRONG_PASSWORD;
        }
        else {
            r = SQLV_SUCCESS;
        }
    }
    else {
        r = SQLV_USER_NOT_FOUND;
    }

    mysql_free_result(res);
    mysql_close(con);

    return r;
}
