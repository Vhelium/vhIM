#include <stdlib.h>
#include <string.h>
#include "server_user.h"

#include "../constants.h"
#include "openssl/ssl.h"

struct server_user *server_user_create(int id, SSL *ssl, char *username,
        unsigned char p_level)
{
    struct server_user *user = malloc(sizeof(struct server_user));
    user->id = id;
    user->con_len = 1;
    user->connections = malloc(sizeof(struct server_user_connection));
    user->connections->ssl = ssl;
    user->connections->next = NULL;
    user->username = strdup(username);
    user->p_level = p_level;

    return user;
}

void server_user_destroy(struct server_user *su)
{
    debugv("destroying user..\n");
    free(su->username);
    su->id = -1;
    /* destroy all items in linked list */
    struct server_user_connection *p = su->connections;
    while(p) {
        struct server_user_connection *old = p;
        old->ssl = NULL;
        p = old->next;

        free(old);
    }
    su->username = NULL;
    free(su);
}

void server_user_add_connection(struct server_user *su, SSL *ssl)
{
    /* create new connection object */
    struct server_user_connection *co = malloc(sizeof(struct server_user_connection));
    co->ssl = ssl;
    /* append to linked list */
    co->next = su->connections;
    su->connections = co;
    su->con_len++;
}

void server_user_remove_connection(struct server_user *su, SSL *ssl)
{
    struct server_user_connection *cur = su->connections, *prev = NULL;
    while (cur && cur->ssl != ssl) {
        prev = cur;
        cur = cur->next;
    }
    if (cur && cur->ssl == ssl) {
        if (prev == NULL) /* begin */
            su->connections = cur->next;
        else
            prev->next = cur->next;
        /* free removed struct */
        free(cur);
        /* decrease connection count */
        su->con_len--;
    }
}
