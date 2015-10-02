#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <stdlib.h>

#include "client_ch.h"
#include "../network/messagetypes.h"
#include "../network/datapacket.h"

#define PORT 55099
#define HOST "vhelium.com"

static int read_line(char str[], int n);

static void process_packet(int fd, byte *data)
{
    datapacket *dp = datapacket_create_from_data(data);
    int packet_type = datapacket_get_int(dp);

    switch(packet_type) {
        case MSG_WELCOME:
        {
            char *msg = datapacket_get_string(dp);
            printf("Server welcomed you.: %s\n", msg);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, "Hi all. I'm Auth'ed :)");
            size_t s = datapacket_finish(answer);
            client_ch_send(answer->data, s);

            free(msg);
        }
        break;

        case MSG_AUTH_FAILED:
        {
            char *msg = datapacket_get_string(dp);
            printf("Server welcomed you.: %s\n", msg);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, "Hi all. I am a failure :/");
            size_t s = datapacket_finish(answer);
            client_ch_send(answer->data, s);

            free(msg);
        }
        break;

        case MSG_BROADCAST:
        {
            char *name = datapacket_get_string(dp);
            char *msg = datapacket_get_string(dp);
            printf("[%s]: %s\n", name, msg);
            free(name);
            free(msg);
        }
        break;

        default:
            printf("Unknown packet: %d\n", packet_type);
            break;
    }

    datapacket_destroy(dp); // destroy the dp with its data array
}

void *process_input()
{
    for(;;)
    {
        char inputBuffer[1024+1];
        int n = read_line(inputBuffer, 1024);
        if (n>0)
        {
            datapacket *dp = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(dp, inputBuffer);
            int s = datapacket_finish(dp);
//            printf("\n\nsending packet:\n");
//            datapacket_dump(dp);

            client_ch_send(dp->data, s);
        }
    }
}

int main(void)
{
    client_ch_start(HOST, PORT);

    datapacket *answer = datapacket_create(MSG_LOGIN);
    datapacket_set_string(answer, "Vhelium");
    size_t s = datapacket_finish(answer);
    client_ch_send(answer->data, s);

    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, process_input, NULL))
    {
        printf("Error creating thread\n");
        return 2;
    }
    else
        printf("input thread started.\n\n");

    client_ch_listen(&process_packet);

    client_ch_destroy();

    datapacket *dp = datapacket_create(0x01);
    datapacket_set_string(dp, "DataPacket test.\n");
    
    return 0;
}

static int read_line(char str[], int n)
{
    int ch, i = 0;
    while(isspace(ch = getchar()))
                    ;
    while(ch != '\n' && ch != EOF)
    {
        if(i < n)
            str[i++] = ch;
        ch = getchar();
    }
    str[i] = '\0';
    return i;
}

