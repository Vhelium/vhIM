#ifndef SERVER_USER_H
#define SERVER_USER_H

#include "openssl/ssl.h"

struct server_user_connection
{
    SSL *ssl;
    struct server_user_connection *next;
};

struct server_user_info
{
    int id;
    char *username;
};

struct server_user
{
    int id;
    size_t con_len;
    struct server_user_connection *connections;
    char *username;
    unsigned char p_level;
};

/* new memory will be allocated for username and the linked list for connections */
struct server_user *server_user_create(int id, SSL *ssl, char *username, unsigned char p_level);

/* has to be invoked manually */
void server_user_destroy(struct server_user *su);

void server_user_add_connection(struct server_user *su, SSL *ssl);

void server_user_remove_connection(struct server_user *su, SSL *ssl);

#endif
