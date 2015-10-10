#include <stdlib.h>
#include <stdio.h>
#include <mysql.h>
#include <string.h>

#include <openssl/sha.h>
#include <openssl/rand.h>

#include "sql_ch.h"
#include "../constants.h"
#include "../utility/strings_helper.h"

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

int sql_check_user_auth(char *user, char *pw, int *result)
{
    MYSQL *con;
    MYSQL_RES *res;
    MYSQL_ROW row;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        *result = SQLV_CONNECTION_ERROR;
        return USER_ID_INVALID;
    }

    //TODO: prevent slq injection via username
    sprintf(query, "SELECT `id`, `password`, `salt` FROM `users` WHERE `user` = '%s' LIMIT 1", user);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        *result = SQLV_CONNECTION_ERROR;
        return USER_ID_INVALID;
    }

    res = mysql_use_result(con);
    int userid = USER_ID_INVALID;

    if ((row = mysql_fetch_row(res)) != NULL) {
        char *uid = row[0];
        char *fetched_pw = row[1];
        char *salt = row[2];

        if (uid == NULL || fetched_pw == NULL || salt == NULL) {
            *result = SQLV_CONNECTION_ERROR;
            goto END;
        }

        /* prepend salt to entered pw */
        char *salt_pw = malloc(strlen(salt) + strlen(pw)+1);
        strcpy(salt_pw, salt);
        strcat(salt_pw, pw);

        /* hash it */
        SHA256_CTX context;
        unsigned char md[SHA256_DIGEST_LENGTH];
        SHA256_Init(&context);
        SHA256_Update(&context, salt_pw, strlen(salt_pw));
        SHA256_Final(md, &context);

        /* get hex representation */
        char hexed_hash[2*SHA256_DIGEST_LENGTH + 1];
        ubytes_to_string(md, sizeof(md), hexed_hash);

        free(salt_pw);

        /* compare hashed input with database entry */
        if (strcmp(fetched_pw, hexed_hash)) {
            *result = SQLV_WRONG_PASSWORD;
        }
        else {
            *result = SQLV_SUCCESS;
            userid = strtol(uid, NULL, 10);
        }
    }
    else {
        *result = SQLV_USER_NOT_FOUND;
    }

END:
    mysql_free_result(res);
    mysql_close(con);

    return userid;
}

int sql_ch_add_user(char *user, char *pw)
{
    int result = SQLV_SUCCESS;
    MYSQL *con;
    MYSQL_RES *res;
    MYSQL_ROW row;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    // check if username already exists:
    
    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `users` WHERE `user` = '%s')", user);
    if (mysql_query(con, query)) {

        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '1') {
            result = SQLV_USER_EXISTS;
            goto QUERY_FAIL;
        }
    }
    else {
        result = SQLV_USER_EXISTS;
        goto QUERY_FAIL;
    }

    mysql_free_result(res);

    if (result == SQLV_SUCCESS) { /* user doesn't exist */
        /* generate random salt */
        unsigned char salt_bytes[32];
        RAND_bytes(salt_bytes, 32);
        /* convert bytes to hex representation */
        char hexed_salt[2*sizeof(salt_bytes) + 1];
        ubytes_to_string(salt_bytes, sizeof(salt_bytes), hexed_salt);

        /* prepend salt to entered pw */
        char *salt_pw = malloc(strlen(hexed_salt) + strlen(pw)+1);
        strcpy(salt_pw, hexed_salt);
        strcat(salt_pw, pw);

        /* hash it */
        SHA256_CTX context;
        unsigned char md[SHA256_DIGEST_LENGTH];
        SHA256_Init(&context);
        SHA256_Update(&context, salt_pw, strlen(salt_pw));
        SHA256_Final(md, &context);
        
        /* convert bytes to readable string -> size doubled */
        char hexed_hash[2*sizeof(md) + 1];
        ubytes_to_string(md, sizeof(md), hexed_hash);

        free(salt_pw);

        //TODO: prevent slq injection via username
        sprintf(query, "INSERT INTO `users` VALUES(DEFAULT, '%s', '%s', '%s')", user, hexed_hash, hexed_salt);

        if (mysql_query(con, query)) {
            errv("%s\n", mysql_error(con));
            result = SQLV_CONNECTION_ERROR;
            goto END;
        }
    }
   
    goto END;

QUERY_FAIL:
    mysql_free_result(res);

END:
    mysql_close(con);

    return result;
}

void sql_ch_destroy()
{
    // nothing to do
}
