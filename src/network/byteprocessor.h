#ifndef BYTEPROCESSOR_H
#define BYTEPROCESSOR_H

#include <stdlib.h>
#include "../constants.h"

/* processes the handled byte data
 * when a whole packet is read, the callback function is called with
 * a newly allocated byte array.
 * params:
 *      per-socket buffers
 * returns:
 *      1, if processing was successfull
 *      0, if processing didn't happen (e.g. negative packet size provided) */
int bp_process_data(byte *data, size_t data_len,byte *rest_buffer,
        size_t *rest_buffer_len, int fd, void (*cb)(int fd, byte *data));

#endif
