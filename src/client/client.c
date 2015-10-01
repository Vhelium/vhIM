#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#include "client_ch.h"
#include "../network/datapacket.h"

#define PORT 55099
#define HOST "vhelium.com"

static void bytes_read(byte *packet)
{
    datapacket *dp = datapacket_create_from_data(packet);
    printf("message received(id=%d): %s", datapacket_get_int(dp), datapacket_get_string(dp));
}

static int read_line(char str[], int n)
{
    int ch, i = 0;
    while(isspace(ch = getchar()))
                    ;
    while(ch != '\n' && ch != EOF)
    {
        if(i < n-1)
            str[i++] = ch;
        ch = getchar();
    }
    str[i] = '\n';
    str[i+1] = '\0';
    return i;
}

void *process_input()
{
    for(;;)
    {
        char inputBuffer[1024+1];
        int n = read_line(inputBuffer, 1024);
        if (n>0)
        {
            datapacket *dp = datapacket_create(0x89abcdef);
            datapacket_set_string(dp, inputBuffer);
            int s = datapacket_finish(dp);
            printf("\n\nsending packet:\n");
            datapacket_dump(dp);

            client_ch_send(dp->data, s);
        }
    }
}

int main(void)
{
    client_ch_start(HOST, PORT);

    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, process_input, NULL))
    {
        printf("Error creating thread\n");
        return 2;
    }
    else
        printf("input thread started.\n\n");

    client_ch_listen(&bytes_read);

    client_ch_destroy();

    datapacket *dp = datapacket_create(0x01);
    datapacket_set_string(dp, "DataPacket test.\n");
    
    return 0;
}
