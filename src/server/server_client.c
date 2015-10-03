#include <stdlib.h>
#include <string.h>
#include "openssl/ssl.h"
#include "server_client.h"

struct server_client *server_client_create(int id, int fd, SSL *ssl)
{
    struct server_client *user = malloc(sizeof(struct server_client));
    user->id = id;
    user->fd = fd;
    user->ssl = ssl;

    return user;
}

void server_client_destroy(struct server_client *sc)
{
    sc->id = -1;
    sc->fd = -1;
    sc->ssl = NULL;
    free(sc);
}
