#ifndef SERVER_USER_H
#define SERVER_USER_H

#include "openssl/ssl.h"

struct server_user
{
    int id;
    SSL *ssl;
    char *username;
};

// new memory will be allocated for username
struct server_user *server_user_create(int id, SSL *ssl, char *username);

// has to be invoked manually
void server_user_destroy(struct server_user *su);

#endif
