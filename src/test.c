#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <ncurses.h>
#include "network/datapacket.h"
#include <openssl/sha.h>
#include "utility/vistack.h"

static void test_int()
{
    int *i = calloc(1, sizeof(int));
    printf("i: %d\n", *i);
    (*i)++;
    printf("i: %d\n", *i);
    *i = 88;
    printf("i: %d\n", *i);

    free(i);
}

static void test_stack()
{
    struct vistack *s = vistack_create();
    vistack_push(s, 0);
    vistack_push(s, 1);
    vistack_push(s, 2);
    vistack_push(s, 3);
    vistack_pop(s);
    vistack_push(s, 4);
    int i = 5;
    for (;i<100;++i)
        vistack_push(s, i);
    while (!vistack_is_empty(s)) {
        printf("pop: %d\n", vistack_pop(s));
    }
}

static void test_salt()
{
    char *salt = "12345678123456781234567812345678";
    char *upw = "123456";
    /* preprend salt to entered */
    char *salt_pw = malloc(strlen(salt) + strlen(upw) + 1);
    strcpy(salt_pw, salt);
    strcat(salt_pw, upw);

    /* hash it */
    SHA256_CTX context;
    unsigned char md[SHA256_DIGEST_LENGTH];
    SHA256_Init(&context);
    SHA256_Update(&context, (unsigned char*)salt_pw, strlen(salt_pw));
    SHA256_Final(md, &context);

    printf("hashed pw:\n");
    int i;
    for(i=0; i<sizeof(md); ++i)
        printf("%02X ", (int)(md)[i]);
    printf("\n\n");

    free(salt_pw);
}

static void test_ncurses()
{
    initscr();
    printw("yoyooyoyoyoy");
    refresh();
    getch();
    endwin();
}

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
    test_int();
}
