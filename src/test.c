#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include "network/datapacket.h"

static void test_dp()
{
    printf("starting the dp test\n");
    datapacket *dp = datapacket_create(0x89abcdef);
    printf("dp initialized with id=0x89abcdef\n");

    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);
    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);
    printf("set_bool: false\n");
    datapacket_set_bool(dp, false);
    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);

    printf("set_string: penis\n");
    datapacket_set_string(dp, "penis");

    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);

    printf("\ndp data set\n\n");

    datapacket_finish(dp);
    datapacket_dump(dp);

    printf("\nreading directly from packet.\n");
    datapacket_reset(dp);
    printf("get_int: %02X\n", datapacket_get_int(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_string: %s\n", datapacket_get_string(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("\ndone.\n");
}

static void test_sql()
{
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *server = "localhost";
    char *user = "vhIM";
    char *pw = "vhInMa";
    char *db = "vhIM";

    printf("version: %s\n", mysql_get_client_info());

    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, server, user, pw, db, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    if (mysql_query(conn, "show tables")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(2);
    }

    res = mysql_use_result(conn);
    printf("res: %p\n", res);

    printf("MySQL tables in db:\n");
    while ((row = mysql_fetch_row(res)) != NULL)
        printf("%s \n", row[0]);

    mysql_free_result(res);
    mysql_close(conn);
}

int main(void)
{
    test_sql();
}
