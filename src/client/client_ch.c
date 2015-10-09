#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../constants.h"
#include "client_ch.h"
#include "../network/datapacket.h"
#include "../constants.h"
#include "../network/byteprocessor.h"

static int sockfd = 0;
static struct sockaddr_in serv_addr;
static struct hostent *server;

SSL *ssl;
SSL_CTX *ctx;

SSL_CTX* init_ctx(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();       /* Load cryptos, et.al. */
    SSL_load_error_strings();           /* Bring in and register error messages */
    method = SSLv3_client_method();     /* Create new client-method instance */
    ctx = SSL_CTX_new(method);          /* Create new context */
    if ( ctx == NULL ) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void show_certs(SSL* ssl)
{   
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);   /* get the server's certificate */
    if ( cert != NULL ) {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);                         /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);                         /* free the malloc'ed string */
        X509_free(cert);                    /* free the malloc'ed certificate copy */
    }
    else
        printf("No certificates.\n");
}

int client_ch_start(char *host, int port)
{
    // initialize SSL
    SSL_library_init();
    ctx = init_ctx();

    // set up TCP connection
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        errv("\nError: couldn't create socket, lel\n");
        return 1;
    }

    server = gethostbyname(host);
    if (server == NULL)
    {
        errv("Error: host not found");
        return 4;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        errv("\nError: connection failed \n");
        return 3;
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    //show_certs(ssl);
    if (SSL_connect(ssl) == -1) {
        errv("couldn't connect to server:\n");
        ERR_print_errors_fp(stderr);
        close(sockfd);
        SSL_CTX_free(ctx);
        return 2;
    }

    return 0;
}

/* Writes @data_len bytes from @data to the server.
 * Note: If invoked with a packet, remember to free it afterwards!
 */
void client_ch_send(byte *data, size_t data_len)
{
    char *sendBuff = (char *)data;
    ssize_t nbytes = SSL_write(ssl, sendBuff, data_len);
    if (!nbytes)
        errv("error sending.\n");
}

void client_ch_listen(callback_msg_rcv_t cb_msg_rcv)
{
    byte data_buffer[1024] = {0};
    ssize_t nbytes = 0; 
    byte rest_buffer[1024] = {0};
    size_t rest_buffer_len = 0;
    int res = 1;

    printf("start listening..\n");

    while (res && (nbytes = SSL_read(ssl, data_buffer, sizeof(data_buffer))) > 0)
    {
        printf("reading!");
        res = bp_process_data(data_buffer, nbytes,
                rest_buffer, &rest_buffer_len, &sockfd, cb_msg_rcv);
    }

    if (nbytes<0)
        errv("read error\n");
    if(!res)
        errv("bullshit detected\n");
}

void client_ch_destroy()
{
    close(sockfd);
    SSL_CTX_free(ctx);

    printf("destroy..\n");
}
