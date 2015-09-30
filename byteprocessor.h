#ifndef BYTEPROCESSOR_H
#define BYTEPROCESSOR_H

#include <stdlib.h>
#include "constants.h"

void bp_process_data(byte *data, size_t data_len, byte *rest_buffer, size_t *rest_buffer_len, void (*cb)(byte *));

#endif
