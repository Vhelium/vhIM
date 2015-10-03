#ifndef SERVER_CH_H
#define SERVER_CH_H

#include "server_client.h"
#include "../network/byteprocessor.h"
#include "../constants.h"

typedef void (*callback_cl_cntd_t)(struct server_client *su);
typedef void (*callback_cl_dc)(struct server_client *su);
 
int server_ch_start(char *port);

void server_ch_send(int fd, byte *data, size_t data_len);

// void server_ch_send_all(int fd_from, byte *data, size_t data_len);

void server_ch_listen(callback_cl_cntd_t, callback_msg_rcv_t, callback_cl_dc);

// inform the connection handler about the auth'ed user's new id
void server_ch_user_authed(int id, int fd);

void server_ch_destroy();

#endif
