#include <stdlib.h>
#include <string.h>
#include "server_user.h"

struct server_user *server_user_create(int id, int fd, char *username)
{
    struct server_user *user = malloc(sizeof(struct server_user));
    user->id = id;
    user->fd = fd;
    user->username = strdup(username);

    return user;
}

void server_user_destroy(struct server_user *su)
{
    free(su->username);
    su->id = -1;
    su->fd = -1;
    su->username = NULL;
    free(su);
}
