#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "constants.h"
#include "byteconverter.h"

int bp_process_data(byte *data, size_t data_len, byte *rest_buffer, size_t *rest_buffer_len, void (*cb)(byte *))
{
    bool go = false;

    printf("bp: data=");
    int i;
    for(i=0; i<data_len; ++i)
        printf("%02X ", (int)(data)[i]);
    printf("\n\n");

    do
    {
        go = false;
        int total_data_len = *rest_buffer_len;
        total_data_len += data_len;
        byte *total_data[total_data_len];
        // handle rest buffer
        memcpy(total_data, rest_buffer, *rest_buffer_len);
        // copy from new buffer
        memcpy(total_data + *rest_buffer_len, data, data_len);

        // get packet size
        byte ia[sizeof(int)];
        memcpy(ia, total_data, sizeof(ia));
        int packet_size = bc_to_int(ia);

        if (packet_size < 0)
        {
            // bullshit detected
            printf("bp: negative packet size: %d\n", packet_size);
            return 0;
        }

        printf("packet size: %d\n", packet_size);

        if (packet_size < total_data_len) // 'read too much'
        {
            byte *packet = malloc(packet_size);
            memcpy(packet, total_data, packet_size);

            cb(packet);

            // copy the overflow into the buffer and adjust its size
            *rest_buffer_len = total_data_len - packet_size;
            memcpy(rest_buffer, total_data + packet_size, *rest_buffer_len);

            go = true;
        }
        else
        {
            if (packet_size == total_data_len)
            {
                // allocate new byte array for package with the data
                byte *packet = malloc(packet_size);
                memcpy(packet, total_data, packet_size);

                cb(packet);
            }
            else
            {
                // copy unfinished packet into the rest buffer
                *rest_buffer_len = total_data_len;
                memcpy(rest_buffer, total_data, *rest_buffer_len);
            }
            go = false;
        }
    }
    while(go);

    return 1;
}
