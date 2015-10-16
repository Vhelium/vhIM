#ifndef SERVER_CH_H
#define SERVER_CH_H

#include "openssl/ssl.h"
#include "server_client.h"
#include "server_user.h"
#include "../network/byteprocessor.h"
#include "../constants.h"

typedef void (*callback_cl_cntd_t)(struct server_client *su);
typedef void (*callback_cl_dc)(struct server_client *su);
 
int server_ch_start(char *port);

/* Writes @data_len bytes from @data to the socket at @ssl
 * Note: If invoked with a datapacket, remember to free it afterwards!
 */
void server_ch_send(SSL *ssl, byte *data, size_t data_len);

void server_ch_listen(callback_cl_cntd_t, callback_msg_rcv_t, callback_cl_dc);

// inform the connection handler about the auth'ed user's new id
void server_ch_client_authed(int id, SSL *ssl);

void server_ch_disconnect_client(SSL *ssl, callback_cl_dc cb_cl_dc);

void server_ch_destroy();

#endif
