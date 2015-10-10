#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include "openssl/ssl.h"
#include "server_user.h"

struct server_client
{
    int id;
    SSL *ssl;
};

struct server_client *server_client_create(int id, SSL *ssl);

int sc_fd(struct server_client *sc);

// has to be invoked manually
void server_client_destroy(struct server_client *sc);

#endif
