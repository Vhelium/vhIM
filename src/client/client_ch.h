#ifndef CLIENT_CH_H
#define CLIENT_CH_H

#include "../constants.h"
#include "../network/byteprocessor.h"
 
int client_ch_start();

void client_ch_send(byte *data, size_t data_len);

void client_ch_listen(callback_msg_rcv_t cb_msg_rcv);

void client_ch_destroy();

#endif
