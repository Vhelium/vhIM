#ifndef BYTECONVERTER_H
#define BYTECONVERTER_H

#include <stdbool.h>
#include "../constants.h"

void bc_from_bool(byte *dest, bool data);

void bc_from_int(byte *dest, int data);

bool bc_to_bool(byte *data);

int bc_to_int(byte *data);

#endif

