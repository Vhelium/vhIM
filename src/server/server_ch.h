#ifndef SERVER_CH_H
#define SERVER_CH_H

#include "../constants.h"
 
int server_ch_start(char *port);

void server_ch_send(int fd, byte *data, size_t data_len);

//void server_ch_send_all(int fd_from, byte *data, size_t data_len);

void server_ch_listen(void (*cb_cl_cntd)(int fd),
        void (*cb_msg_rcv)(int fd, byte *data),
        void (*cb_cl_dc)(int fd));

void server_ch_destroy();

#endif
