#include <stdlib.h>
#include <string.h>
#include "server_user.h"

#include "openssl/ssl.h"

struct server_user *server_user_create(int id, SSL *ssl, char *username)
{
    struct server_user *user = malloc(sizeof(struct server_user));
    user->id = id;
    user->ssl = ssl;
    user->username = strdup(username);

    return user;
}

void server_user_destroy(struct server_user *su)
{
    free(su->username);
    su->id = -1;
    su->ssl = NULL;
    su->username = NULL;
    free(su);
}
