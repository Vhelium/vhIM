#include <stdbool.h>
#include "constants.h"

void bc_from_bool(byte *dest, bool data)
{
    dest[0] = (data ? 0x01 : 0x00); // 1 byte
}

void bc_from_int(byte *dest, int data)
{
    dest[0] = ((data >> 24) & 0xff);
    dest[1] = ((data >> 16) & 0xff);
    dest[2] = ((data >> 8) & 0xff);
    dest[3] = ((data >> 0) & 0xff);
}

bool bc_to_bool(byte *data)
{
    return !data ? false : data[0] != 0x00; 
}

int bc_to_int(byte *data)
{
    return (0xff & data[0]) << 24 | (0xff & data[1]) << 16 | (0xff & data[2]) << 8 | (0xff & data[3]) << 0;
}

