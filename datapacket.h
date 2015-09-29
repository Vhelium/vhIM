#ifndef DATAPACKET_H
#define DATAPACKET_H

#include <stdbool.h>

typedef struct Datapacket
{
    byte data[];
    int index;
} datapacket;

datapacket *datapacket_create(int type);

void datapacket_set_boolean(datapacket *dp, bool b);
void datapacket_set_int(datapacket *dp, int i);
void datapacket_set_long(datapacket *dp, long l);
void datapacket_set_string(datapacket *dp, char *s);

bool datapacket_get_boolean(datapacket *dp);
int datapacket_get_int(datapacket *dp);
long datapacket_get_long(datapacket *dp);
char *datapacket_get_string(datapacket *dp);

#endif

