#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "../constants.h"
#include "datapacket.h"
#include "byteconverter.h"

datapacket *datapacket_create(int type)
{
    datapacket *dp = malloc(sizeof(datapacket));
    dp->index = sizeof(int);
    dp->buf_size = PACKET_SIZE;
    dp->data = malloc(dp->buf_size);
    datapacket_set_int(dp, type);
    return dp;
}

datapacket *datapacket_create_from_data(byte *data)
{
    datapacket *dp = malloc(sizeof(datapacket));
    dp->index = sizeof(int);
    dp->buf_size = sizeof(data);
    dp->data = data;
    return dp;
}

static void datapacket_verify_size(datapacket *dp, size_t additional)
{
    size_t cur_size = dp->index;                    // currently used bytes
    assert(cur_size <= dp->buf_size);

    if(dp->buf_size - cur_size > additional)        // no need for more storage
        return;

    while(dp->buf_size - cur_size <= additional)    // while more space needed
        dp->buf_size += PACKET_SIZE;                // increment by PACKET_SIZE

    dp->data = realloc(dp->data, dp->buf_size);   // allocate new data array
}

void datapacket_set_bool(datapacket *dp, bool b)
{
    byte ba[1];                             // buffer for bool as byte[]
    bc_from_bool(ba, b);                    // copy into buffer

    datapacket_verify_size(dp, 1);

    memcpy(dp->data + dp->index, ba, 1);    // copy new data

    dp->index += 1;                         // increase index
}

void datapacket_set_int(datapacket *dp, int i)
{
    byte ba[sizeof(int)];
    bc_from_int(ba, i);

    datapacket_verify_size(dp, sizeof(int));

    memcpy(dp->data + dp->index, ba, sizeof(int));    // copy new data

    dp->index += sizeof(int);
}

int datapacket_simulate_int(datapacket *dp)
{
    datapacket_verify_size(dp, sizeof(int));
    dp->index += sizeof(int);
    return dp->index - sizeof(int);
}

void datapacket_update_int(datapacket *dp, int index, int i)
{
    if (index >= 0 && index < dp->index) {
        byte ba[sizeof(int)];
        bc_from_int(ba, i);

        memcpy(dp->data + index, ba, sizeof(int));    // copy new data

        dp->index += sizeof(int);
    }
}

void datapacket_set_long(datapacket *dp, long l)
{

}

void datapacket_set_string(datapacket *dp, const char *s)
{
    int len = strlen(s);

    datapacket_verify_size(dp, sizeof(int) + sizeof(char) * (len+1));

    datapacket_set_int(dp, len);    // set integer with string length (in terms of char)
    memcpy(dp->data + dp->index, s, len+1);    // copy string (including \0)
    
    dp->index += len + 1;
}

bool datapacket_get_bool(datapacket *dp)
{
    byte ba[1];
    memcpy(ba, dp->data + dp->index, 1);
    bool b = bc_to_bool(ba);
    dp->index++;
    return b;
}

int datapacket_get_int(datapacket *dp)
{
    byte ba[sizeof(int)];
    memcpy(ba, dp->data + dp->index, sizeof(int));
    int i = bc_to_int(ba);
    dp->index += sizeof(int);
    return i;
}

long datapacket_get_long(datapacket *dp)
{
    //TODO
    return 0;
}

char *datapacket_get_string(datapacket *dp)
{
    int len = datapacket_get_int(dp);
    byte *sa = malloc(sizeof(char) * (len+1));  // allocate memory for string
    memcpy(sa, dp->data + dp->index, sizeof(char) * (len+1));
    char *s = (char *)sa;
    dp->index += (len+1) * sizeof(char);

    return s;
}

void datapacket_reset(datapacket *dp)
{
    dp->index = sizeof(int); // first 4 bytes are used to indicate packet size!
}

int datapacket_finish(datapacket *dp)
{
    int size = dp->index;
    byte ia[sizeof(int)];
    bc_from_int(ia, size);
    memcpy(dp->data, ia, sizeof(int));
    return size;
}

void datapacket_dump(datapacket *dp)
{
    size_t index = dp->index;
    size_t buf_size = dp->buf_size;

    datapacket_reset(dp);                        // reset for reading
    int messageId = datapacket_get_int(dp);
    dp->index = index;                           // set index back

    printf("datapacket(id = %X):\n", messageId);
    printf("size: %lu/%lu\n", index, buf_size);
    printf("data: ");
    int i;
    for(i=0; i<index; ++i)
        printf("%02X ", (int)(dp->data)[i]);
    printf("\n");
}

void datapacket_destroy(datapacket *dp)
{
    free(dp->data);
    dp->data = NULL;
    free(dp);
}
