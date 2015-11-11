#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "gdsl_types.h"
#include "gdsl_rbtree.h"

#include "openssl/ssl.h"
#include "openssl/err.h"

#include "server_ch.h"
#include "server_client.h"

#include "../constants.h"
#include "../network/datapacket.h"
#include "../network/byteprocessor.h"

/* master file descriptor list */
static fd_set master;
/* temp file descriptor list for select() */
static fd_set read_fds;
/* maximum file descriptor number */
static int fdmax;

/* listening socket descriptor */
static int listener;

/* client management */
static gdsl_rbtree_t clients; // list of all connected clients
static void server_ch_client_connected(int fd, SSL *ssl, callback_cl_cntd_t cb_cl_cntd);
static void server_ch_client_disconnected(int fd, callback_cl_dc cb_cl_dc);

/* SSL variables */
static SSL_CTX *ctx;
static SSL_CTX *init_server_ctx(void);
static void load_certificates(SSL_CTX* ctx, char* CertFile, char* KeyFile);
static void show_certs(SSL *ssl);

/* compare [server_client] with [server_client] */
static long int compare_client_fd(const gdsl_element_t E, void *VALUE)
{
    return sc_fd((struct server_client *)E) - sc_fd((struct server_client *)VALUE);
}

/* compare [server_client] with [fd] */
static long int compare_client_fd_directly(const gdsl_element_t E, void *VALUE)
{
    return sc_fd((struct server_client *)E) - *((int *)VALUE);
}

static struct server_client *get_client_by_fd(int fd)
{
    return gdsl_rbtree_search(clients, &compare_client_fd_directly, &fd);
}

static void gdsl_rbtree_clients_free(gdsl_element_t E)
{
    server_client_destroy((struct server_client *)E);
}

int server_ch_start(char *port)
{
    // init datastructures
    clients = gdsl_rbtree_alloc("CLIENTS", NULL, &gdsl_rbtree_clients_free, &compare_client_fd);

    // initialize SSL
    SSL_library_init();
    ctx = init_server_ctx();
    load_certificates(ctx, "vhIM.crt", "vhIM.key");

    // set up TCP socket
    int rv, yes=1;      // for setsockopt()
    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);   // clear master and temp sets
    FD_ZERO(&read_fds);

    // get the socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(listener < 0)
            continue;

        // ignore "adress already in use" spam
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if(bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // check if socket is bound
    if(p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // no need for it again

    // listen
    if(listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add listener to master set
    FD_SET(listener, &master);

    // keep track of biggest file descriptor'
    fdmax = listener; //currently it's this one

    return 0;
}

void server_ch_send(SSL *ssl, byte *data, size_t data_len)
{
    if (SSL_write(ssl, data, data_len) == -1)
        perror("send");
}

void server_ch_listen(callback_cl_cntd_t cb_cl_cntd,
        callback_msg_rcv_t cb_msg_rcv,
        callback_cl_dc cb_cl_dc)
{
    byte data_buffer[1024] = {0};
    ssize_t nbytes = 0; 
    byte rest_buffer[1024] = {0};
    size_t rest_buffer_len = 0;
    int res = 1;

    int newfd;                          // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; //client address
    socklen_t addrlen;

    int i;

    // main loop
    for(;;) {
        read_fds = master;  // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through existing connections looking for data to read
        for(i = 0; i <= fdmax; ++i)
            if(FD_ISSET(i, &read_fds)) { // found sth
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1)
                        perror("accept");
                    else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax)      // keep track of max
                            fdmax = newfd;

                        // handle new connection
                        SSL *ssl = SSL_new(ctx);
                        SSL_set_fd(ssl, newfd);

                        if (SSL_accept(ssl) == -1) {
                            ERR_print_errors_fp(stderr);

                            // clean up connection
                            FD_CLR(newfd, &master);
                            SSL_free(ssl);
                            close(i);
                        }
                        else {
                            server_ch_client_connected(newfd, ssl, cb_cl_cntd);
                        }
                    }
                }
                else {
                    // handle data from a client
                    SSL *ssl = get_client_by_fd(i)->ssl;
                    if ((nbytes = SSL_read(ssl, data_buffer,
                                    sizeof(data_buffer))) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        }
                        else
                            perror("recv");

                        server_ch_client_disconnected(i, cb_cl_dc);

                        // clean up connection
                        FD_CLR(i, &master);
                        SSL_free(ssl);
                        close(i);
                    }
                    else {
                        debugv("bytes received: %ld\n", nbytes);
                        res = bp_process_data(data_buffer, nbytes,
                                rest_buffer, &rest_buffer_len,
                                gdsl_rbtree_search(clients, &compare_client_fd_directly, &i),
                                cb_msg_rcv);

                        if (!res)
                            perror("processing data");
                    }
                }
            }
    }// end for(;;)
}

/* Gets called when client is authenticated as a user
 * will update the client's userID
 */
void server_ch_client_authed(int id, SSL *ssl)
{
    int fd = SSL_get_fd(ssl);
    struct server_client *sc = gdsl_rbtree_search(clients,
            &compare_client_fd_directly, &fd);
    if (sc != NULL)
        sc->id = id;
}

static SSL_CTX *init_server_ctx(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();       /* load & register all cryptos, etc. */
    SSL_load_error_strings();           /* load all error messages */
    method = TLSv1_1_server_method();     /* create new server-method instance */
    ctx = SSL_CTX_new(method);          /* create new context from method */
    if ( ctx == NULL ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

static void load_certificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) ) {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

static void show_certs(SSL *ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);   /* Get certificates (if available) */
    if ( cert != NULL ) {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}

static void server_ch_client_connected(int fd, SSL *ssl, callback_cl_cntd_t cb_cl_cntd)
{
    int rc;
    struct server_client *sc = server_client_create(-1, ssl);
    gdsl_rbtree_insert(clients, (void *)sc, &rc);
    cb_cl_cntd(sc);
}

static void server_ch_client_disconnected(int fd, callback_cl_dc cb_cl_dc)
{
    struct server_client *sc = gdsl_rbtree_search(clients,
           &compare_client_fd_directly, &fd);
    cb_cl_dc(sc);
    if (sc != NULL) {
        gdsl_rbtree_remove(clients, sc);
        server_client_destroy(sc);
    }
}

void server_ch_disconnect_client(SSL *ssl, callback_cl_dc cb_cl_dc)
{
    int i = SSL_get_fd(ssl);
    server_ch_client_disconnected(i, cb_cl_dc);

    // clean up connection
    FD_CLR(i, &master);
    SSL_free(ssl);
    close(i);
}

void server_ch_destroy()
{
    SSL_CTX_free(ctx);
    gdsl_rbtree_free(clients);
}
