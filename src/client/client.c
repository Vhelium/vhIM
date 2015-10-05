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

#include "../constants.h"
#include "client_ch.h"
#include "../network/messagetypes.h"
#include "../network/datapacket.h"

#define PORT 55099
#define HOST "vhelium.com"

static int read_line(char str[], int n);

/* Send datapacket to server
 * Automatically frees the datapacket afterwards
 */
static int send_to_server(datapacket *dp)
{
    size_t s = datapacket_finish(dp);
    client_ch_send(dp->data, s);
    datapacket_destroy(dp);

    return 0;
}

static void process_packet(void *sender, byte *data)
{
    datapacket *dp = datapacket_create_from_data(data);
    int packet_type = datapacket_get_int(dp);

    switch(packet_type) {
        case MSG_WELCOME:
        {
            char *msg = datapacket_get_string(dp);
            printf("[INFO]: Server welcomes you: %s\n", msg);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, "Hi all. I'm Auth'ed :)");
            send_to_server(answer);

            free(msg);
        }
        break;

        case MSG_AUTH_FAILED:
        {
            char *msg = datapacket_get_string(dp);
            int res = datapacket_get_int(dp);
            printf("[INFO]: %s\nError Code: %d\n", msg, res);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, "Hi all. I am a failure :/");
            send_to_server(answer);

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
            errv("Unknown packet: %d\n", packet_type);
            break;
    }

    datapacket_destroy(dp); // destroy the dp with its data array
}

void *process_input()
{
    for(;;) {
        char inputBuffer[1024+1];
        int n = read_line(inputBuffer, 1024);
        if (n>0) {
            datapacket *dp = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(dp, inputBuffer);
            send_to_server(dp);
        }
    }
}

int main(int argc, char *argv[])
{
    printf("Enter a username: ");
    char nBuf[126+1];
    int n = 0;
    while (n <= 0)
        n = read_line(nBuf, 126);

    printf("\nEnter password: ");
    char pBuf[126+1];
    n = 0;
    while (n <= 0)
        n = read_line(pBuf, 126);

    printf("\n\n");

    char *host = HOST;
    if (argc >= 2)
        host = argv[1];

    if (client_ch_start(host, PORT))
        exit(1);

    datapacket *answer = datapacket_create(MSG_REQ_LOGIN);
    datapacket_set_string(answer, nBuf);
    datapacket_set_string(answer, pBuf);
    send_to_server(answer);

    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, process_input, NULL)) {
        errv("Error creating thread\n");
        return 2;
    }
    else
        debugv("input thread started.\n\n");

    client_ch_listen(&process_packet);

    client_ch_destroy();
    
    return 0;
}

static int read_line(char str[], int n)
{
    int ch, i = 0;
    while(isspace(ch = getchar()))
                    ;
    while(ch != '\n' && ch != EOF) {
        if(i < n)
            str[i++] = ch;
        ch = getchar();
    }
    str[i] = '\0';
    return i;
}
