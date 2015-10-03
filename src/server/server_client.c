#include <stdlib.h>
#include <string.h>
#include "openssl/ssl.h"
#include "server_client.h"

struct server_client *server_client_create(int id, SSL *ssl)
{
    struct server_client *user = malloc(sizeof(struct server_client));
    user->id = id;
    user->ssl = ssl;

    return user;
}

int sc_fd(struct server_client *sc)
{
    printf("sc_fd(): %p resulting in %d\n\n", sc->ssl, SSL_get_fd(sc->ssl));
    return SSL_get_fd(sc->ssl);
}

void server_client_destroy(struct server_client *sc)
{
    sc->id = -1;
    sc->ssl = NULL;
    free(sc);
}
