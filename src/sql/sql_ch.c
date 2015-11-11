#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <string.h>

#include <openssl/sha.h>
#include <openssl/rand.h>

#include "sql_ch.h"
#include "../constants.h"
#include "../utility/strings_helper.h"
#include "../utility/vstack.h"
#include "../utility/vistack.h"
#include "../server/server_user.h"
#include "../server/server_group.h"

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

int sql_ch_check_user_auth(char *user, char *pw, int *result)
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
        result = SQLV_CONNECTION_ERROR;
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
        sprintf(query, "INSERT INTO `users` VALUES(DEFAULT, '%s', '%s', '%s', '0')", user, hexed_hash, hexed_salt);

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

unsigned char sql_ch_load_privileges(int uid)
{
    MYSQL *con;
    MYSQL_RES *res;
    MYSQL_ROW row;

    unsigned char priv = 0;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    sprintf(query, "SELECT `privilege` FROM `users` WHERE `id`='%d'", uid);
    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        priv = atoi(row[0]);
    }
    else {
        goto QUERY_FAIL;
    }

QUERY_FAIL:
    mysql_free_result(res);

END:
    mysql_close(con);

    return priv;
}

int sql_ch_update_privileges(int uid, unsigned char p)
{
    MYSQL *con;
    int r = 0;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        r = 2;
        goto END;
    }

    sprintf(query, "UPDATE `users` SET `privilege`='%u' WHERE `id`='%d'",
            (unsigned int)p, uid);
    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        r = 3;
        goto END;
    }

END:
    mysql_close(con);

    return r;
}

/* creates new friend request if it doesn't exist yet.
 * One has to check first if users might already be friends. */
int sql_ch_create_friend_request(int uid_from, int uid_to)
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

    /* check if there is already a friend request created */
    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `friend_requests` WHERE `friend_id1` = '%d' AND `friend_id2` = '%d')",
            uid_from, uid_to);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '1') {
            result = SQLV_ENTRY_EXISTS;
            goto QUERY_FAIL;
        }
    }
    else {
        result = SQLV_NOPE;
        goto QUERY_FAIL;
    }

    mysql_free_result(res);

    /* check if the other user already requested one previously */
    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `friend_requests` WHERE `friend_id1` = '%d' AND `friend_id2` = '%d')",
            uid_to, uid_from);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '1') {
            debugv("fr: 1\n");
            result = SQLV_OTHER_EXISTS;
            goto QUERY_FAIL;
        }
    }
    else {
        result = SQLV_CONNECTION_ERROR;
        goto QUERY_FAIL;
    }

QUERY_FAIL:
    mysql_free_result(res);

    if (result == SQLV_SUCCESS) { /* request doesn't exist and other user
                                     hasn't requested previoysly */
        //TODO: prevent slq injection
        sprintf(query, "INSERT INTO `friend_requests` VALUES(DEFAULT, '%d', '%d')",
                uid_from, uid_to);

        if (mysql_query(con, query)) {
            errv("%s\n", mysql_error(con));
            result = SQLV_CONNECTION_ERROR;
            goto END;
        }
    }
    else
        goto END;

END:
    mysql_close(con);

    return result;
}

int sql_ch_get_friend_requests(int uid_from, struct vstack *requests_out)
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

    /* query for all pending friend requests */
    sprintf(query, "SELECT friend_requests.friend_id1, users.user FROM `friend_requests` INNER JOIN `users` on users.id = friend_requests.friend_id1 WHERE friend_requests.friend_id2 = '%d'", uid_from);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    res = mysql_use_result(con);

    while ((row = mysql_fetch_row(res)) != NULL) {
        char *uid_f2 = row[0];
        // convert to int
        int uid = atoi(uid_f2);
        // fetch friend's user name
        char *uname = row[1];
        // save in a list
        struct server_user_info *info = malloc(sizeof(struct server_user_info));
        info->username = malloc(strlen(uname) + 1);
        strcpy(info->username, uname);
        info->id = uid;

        vstack_push(requests_out, info);
    }

    mysql_free_result(res);

END:
    mysql_close(con);

    return result;
}

/* creates two entries for friendship (in both directions)
 * does NOT check for existing friendship! */
int sql_ch_create_friends(int uid_1, int uid_2)
{
    int result = SQLV_SUCCESS;
    MYSQL *con;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    /* perform two queries */

    //TODO: prevent slq injection
    sprintf(query, "INSERT INTO `friends` VALUES(DEFAULT, '%d', '%d')",
            uid_1, uid_2);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    //TODO: prevent slq injection
    sprintf(query, "INSERT INTO `friends` VALUES(DEFAULT, '%d', '%d')",
            uid_2, uid_1);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    /* clean up pending friend requests */
    sprintf(query, "DELETE FROM `friend_requests` WHERE (`friend_id1` = '%d' AND `friend_id2` = '%d') OR (`friend_id1` = '%d' AND `friend_id2` = '%d')",
            uid_1, uid_2, uid_2, uid_1);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

END:
    mysql_close(con);

    return result;
}

bool sql_ch_are_friends(int uid_from, int uid_to)
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

    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `friends` WHERE `friend_id1` = '%d' AND `friend_id2` = '%d')",
            uid_from, uid_to);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '1') {
            result = SQLV_ENTRY_EXISTS;
            goto QUERY_FAIL;
        }
    }
    else {
        result = SQLV_NOPE;
        goto QUERY_FAIL;
    }

QUERY_FAIL:
    mysql_free_result(res);

END:
    mysql_close(con);

    return result == SQLV_ENTRY_EXISTS;
}

int sql_ch_get_friends(int uid_from, struct vstack *friends_out)
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

    /* retrieve all friends */
    sprintf(query, "select friends.friend_id2, users.user from `friends` INNER JOIN `users` on users.id = friends.friend_id2 WHERE friends.friend_id1 = '%d'",
            uid_from);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    res = mysql_use_result(con);

    while ((row = mysql_fetch_row(res)) != NULL) {
        // fetch uid
        char *uid_str = row[0];
        // convert to int
        int uid = atoi(uid_str);
        // fetch friend's user name
        char *uname = row[1];
        // save in a list
        struct server_user_info *info = malloc(sizeof(struct server_user_info));
        info->username = malloc(strlen(uname) + 1);
        strcpy(info->username, uname);
        info->id = uid;

        vstack_push(friends_out, info);
    }

    mysql_free_result(res);

END:
    mysql_close(con);

    // return the list
    return result;
}

int sql_ch_delete_friends(int uid_1, int uid_2)
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

    /* check if already friends */
    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `friends` WHERE `friend_id1` = '%d' AND `friend_id2` = '%d')",
            uid_1, uid_2);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '1') {
            result = SQLV_ENTRY_EXISTS;
        }
        else
            result = SQLV_SUCCESS;
    }
    else {
        result = SQLV_CONNECTION_ERROR;
    }

    mysql_free_result(res);

    if (result == SQLV_ENTRY_EXISTS) { /* 1 & 2 are friends */
        // remove friend ship
        sprintf(query, "DELETE FROM `friends` WHERE (`friend_id1` = '%d' AND `friend_id2` = '%d') OR (`friend_id1` = '%d' AND `friend_id2` = '%d')",
                uid_1, uid_2, uid_2, uid_1);

        if (mysql_query(con, query)) {
            errv("%s\n", mysql_error(con));
            result = SQLV_CONNECTION_ERROR;
            goto END;
        }
    }
    else if (result == SQLV_SUCCESS) {
        // otherwise delete possible pending requests
        sprintf(query, "DELETE FROM `friend_requests` WHERE (`friend_id1` = '%d' AND `friend_id2` = '%d') OR (`friend_id1` = '%d' AND `friend_id2` = '%d')",
                uid_1, uid_2, uid_2, uid_1);

        if (mysql_query(con, query)) {
            errv("%s\n", mysql_error(con));
            result = SQLV_CONNECTION_ERROR;
            goto END;
        }
    }

END:
    mysql_close(con);

    return result;
}

bool sql_ch_user_exists(int uid)
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

    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `users` WHERE `id` = '%d')", uid);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '1') {
            result = SQLV_ENTRY_EXISTS;
        }
        else {
            result = SQLV_NOPE;
        }
    }
    else {
        result = SQLV_NOPE;
    }

    mysql_free_result(res);

END:
    mysql_close(con);

    return result == SQLV_ENTRY_EXISTS;
}

int sql_ch_create_group(const char *name, int uid_owner, int *gid)
{
    int result = SQLV_SUCCESS;
    MYSQL *con;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    //TODO: prevent slq injection
    sprintf(query, "INSERT INTO `groups` VALUES(DEFAULT, '%s', '%d')", name, uid_owner);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    *gid = mysql_insert_id(con);

END:
    mysql_close(con);

    return result;
}

int sql_ch_delete_group(int gid)
{
    //TODO
    return 0;
}

int sql_ch_pass_group_ownership(int gid, int uid_old_owner)
{
    //TODO
    return 0;
}

bool sql_ch_is_group_owner(int gid, int uid)
{
    //TODO
    return true;
}

int sql_ch_add_user_to_group(int gid, int uid)
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

    /* check if grp exists */
    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `groups` WHERE `id` = '%d')",
            gid);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '0') {
            result = SQLV_NOPE;
            goto QUERY_FAIL;
        }
    }
    else {
        result = SQLV_NOPE;
        goto QUERY_FAIL;
    }

    mysql_free_result(res);

    /* check if user is already in this group */
    sprintf(query, "SELECT EXISTS(SELECT 1 FROM `users_to_group` WHERE `user_id` = '%d' AND `group_id` = '%d')",
            uid, gid);

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        if (row[0][0] == '1') {
            result = SQLV_ENTRY_EXISTS;
            goto QUERY_FAIL;
        }
    }
    else {
        result = SQLV_NOPE;
        goto QUERY_FAIL;
    }

QUERY_FAIL:
    mysql_free_result(res);

    if (result == SQLV_SUCCESS) { /* not already in group */
        //TODO: prevent slq injection
        sprintf(query, "INSERT INTO `users_to_group` VALUES(DEFAULT, '%d', '%d')",
                uid, gid);

        if (mysql_query(con, query)) {
            errv("%s\n", mysql_error(con));
            result = SQLV_CONNECTION_ERROR;
            goto END;
        }
    }
    else
        goto END;

END:
    mysql_close(con);

    return result;
}

int sql_ch_remove_user_from_group(int gid, int uid)
{
    //TODO
    return 0;
}


int sql_ch_get_groups_of_user(int uid, struct vistack **g)
{
    struct vistack *grps = vistack_create();
    
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

    /* retrieve all groups for user */
    sprintf(query, "SELECT group_id FROM `users_to_group` WHERE `user_id` = '%d'", uid); 

    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        result = SQLV_CONNECTION_ERROR;
        goto END;
    }

    res = mysql_use_result(con);

    while ((row = mysql_fetch_row(res)) != NULL) {
        // fetch gid
        char *gid_str = row[0];
        int gid = atoi(gid_str);

        vistack_push(grps, gid);
    }

    mysql_free_result(res);

END:
    mysql_close(con);

    *g = grps;

    // return the list
    return result;
}

int sql_ch_initialize_group(int gid, struct server_group *grp)
{
    MYSQL *con;
    MYSQL_RES *res;
    MYSQL_ROW row;
    int result = SQLV_FAILURE;

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, sql_con.server, sql_con.user, sql_con.pw, sql_con.db,
                0, NULL, 0)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    sprintf(query, "SELECT * FROM `groups` WHERE `id`='%d'", gid);
    if (mysql_query(con, query)) {
        errv("%s\n", mysql_error(con));
        goto END;
    }

    res = mysql_use_result(con);

    if ((row = mysql_fetch_row(res)) != NULL) {
        int id = atoi(row[0]);
        debugv("Initializing group(%d)\n", id);
        char *name = row[1];
        int owner_id = atoi(row[2]);

        server_group_initialize(grp, id, name, owner_id);
        result = SQLV_SUCCESS;
    }
    else {
        goto QUERY_FAIL;
    }

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
