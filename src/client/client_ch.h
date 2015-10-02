#ifndef CLIENT_CH_H
#define CLIENT_CH_H

#include "../constants.h"
 
int client_ch_start();

void client_ch_send(byte *data, size_t data_len);

void client_ch_listen(void (*callback)(int fd, byte *data));

void client_ch_destroy();

#endif
