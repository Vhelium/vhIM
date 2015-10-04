#ifndef DATAPACKET_H
#define DATAPACKET_H

#include <stdbool.h>
#include <stdio.h>
#include "../constants.h"

#define PACKET_SIZE 32 

typedef struct
{
    byte *data;
    size_t index; // current index, in bytes
    size_t buf_size; // current buffer size, in bytes
} datapacket;

datapacket *datapacket_create(int type);
datapacket *datapacket_create_from_data(byte *data);

void datapacket_set_bool(datapacket *dp, bool b);
void datapacket_set_int(datapacket *dp, int i);
void datapacket_set_long(datapacket *dp, long l);
void datapacket_set_string(datapacket *dp, char *s);

bool datapacket_get_bool(datapacket *dp);
int datapacket_get_int(datapacket *dp);
long datapacket_get_long(datapacket *dp);

/* returns a newly allocated string.
 * Note: remember to free it!
 */
char *datapacket_get_string(datapacket *dp);

void datapacket_reset(datapacket *dp);
int datapacket_finish(datapacket *dp);
void datapacket_dump(datapacket *dp);
void datapacket_destroy(datapacket *dp);

#endif
