#ifndef BYTEPROCESSOR_H
#define BYTEPROCESSOR_H

#include <stdlib.h>
#include "../constants.h"

typedef void (*callback_msg_rcv_t)(void *sender, byte *data);

/* processes the handled byte data
 * when a whole packet is read, the callback function is called with
 * a newly allocated byte array.
 * params:
 *      per-socket buffers
 * returns:
 *      1, if processing was successfull
 *      0, if processing didn't happen (e.g. negative packet size provided) */
int bp_process_data(byte *data, ssize_t data_len,byte *rest_buffer,
        size_t *rest_buffer_len, void *sender, callback_msg_rcv_t cb_msg_rcv);

#endif
