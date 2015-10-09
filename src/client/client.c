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

#include "../utility/command_parser.h"
#include "../utility/strings_helper.h"

#include "../constants.h"
#include "client_ch.h"
#include "../network/messagetypes.h"
#include "../network/datapacket.h"

#define PORT 55099
#define HOST "vhelium.com"

static char arg_host[128] = {0};
static int arg_port;

static int client_connect(char *server, int port);

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
        case MSG_WELCOME:  {
            char *msg = datapacket_get_string(dp);
            printf("[INFO]: Server welcomes you: %s\n", msg);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, "Hi all. I'm Auth'ed :)");
            send_to_server(answer);

            free(msg);
        }
        break;

        case MSG_AUTH_FAILED:  {
            char *msg = datapacket_get_string(dp);
            int res = datapacket_get_int(dp);
            printf("[INFO]: %s\nError Code: %d\n", msg, res);

            datapacket *answer = datapacket_create(MSG_BROADCAST);
            datapacket_set_string(answer, "Hi all. I am a failure :/");
            send_to_server(answer);

            free(msg);
        }
        break;

        case MSG_BROADCAST: {
            char *name = datapacket_get_string(dp);
            char *msg = datapacket_get_string(dp);
            printf("[%s]: %s\n", name, msg);
            free(name);
            free(msg);
        }
        break;

        case MSG_REGISTR_SUCCESSFUL: {
            char *msg = datapacket_get_string(dp);
            printf("%s\n", msg);

            free(msg);
        }

        case MSG_REGISTR_FAILED: {
            char *msg = datapacket_get_string(dp);
            printf("[server]: %s\n", msg);
            free(msg);
        }
        break;

        case MSG_WHO: {
            int i, count = datapacket_get_int(dp);
            printf("Users online: %d\n", count);
            for (i=0; i<count; ++i) {
                char *u = datapacket_get_string(dp);
                printf("    %s\n", u);
                free(u);
            }
            printf("\n\n");
        }
        break;

        default:
            errv("Unknown packet: %d\n", packet_type);
            break;
    }

    datapacket_destroy(dp); // destroy the dp with its data array
}

static int execute_command(int type, char *argv[])
{
    switch (type)
    {
        case MSG_CMD_KICK_ID: {
            //TODO: check if it's a number
            int uid = atoi(argv[0]);
            printf("uid: %d\n", uid);
            datapacket *dp = datapacket_create(MSG_CMD_KICK_ID);
            datapacket_set_int(dp, uid);
            send_to_server(dp);
        }
        break;

        case MSG_WHISPER: {
            //TODO: check if it's a number
            int uid = atoi(argv[0]);
            datapacket *dp = datapacket_create(MSG_WHISPER);
            datapacket_set_int(dp, uid);
            datapacket_set_string(dp, argv[1]);
            send_to_server(dp);
        }
        break;

        case MSG_REQ_REGISTER: {
            datapacket *dp = datapacket_create(MSG_REQ_REGISTER);
            datapacket_set_string(dp, argv[0]);
            datapacket_set_string(dp, argv[1]);
            send_to_server(dp);
        }
        break;

        case MSG_REQ_LOGIN: {
            datapacket *dp = datapacket_create(MSG_REQ_LOGIN);
            datapacket_set_string(dp, argv[0]);
            datapacket_set_string(dp, argv[1]);
            send_to_server(dp);
        }
        break;

        case MSG_LOGOUT: {
            datapacket *dp = datapacket_create(MSG_LOGOUT);
            send_to_server(dp);
        }
        break;

        case MSG_WHO: {
            datapacket *dp = datapacket_create(MSG_WHO);
            send_to_server(dp);
        }
        break;

        case CMD_CONNECT: {
            int port = argv[1] != NULL ? atoi(argv[1]) : PORT;
            if (client_connect(argv[0], port)) {
                printf("error connecting to host\n");
                exit(1);
            }
        }
        break;

        default:
            printf("Invalid command: %d", type);
            return 1;
    }

    return 0;
}

/* ============== THREADING ========================================= */

static bool is_connected = false;

static pthread_t input_thread;
static pthread_mutex_t mutex_connected;

static void set_is_connected_synced(bool b)
{
    pthread_mutex_lock(&mutex_connected);
      is_connected = b;
    pthread_mutex_unlock(&mutex_connected);
}

static bool get_is_connected_synced()
{
    bool b;
    pthread_mutex_lock(&mutex_connected);
      b = is_connected;
    pthread_mutex_unlock(&mutex_connected);
    return b;
}

void *threaded_connect(void *arg)
{
    if (client_ch_start(arg_host, arg_port)) {
        set_is_connected_synced(false);
        return (void *)1;
    }
    else
        set_is_connected_synced(true);

    //TODO: check the flag in the listener loop (thread safe!)
    client_ch_listen(&process_packet);

    client_ch_destroy();
    printf("disconnected.\n");

    set_is_connected_synced(false);

    return NULL;
}

static int client_connect(char *host, int port)
{
    if (!host)
        memcpy(arg_host, HOST, strlen(HOST));
    else
        memcpy(arg_host, host, strlen(host));
    if (!port)
        arg_port = PORT;
    else
        arg_port = port;

    /* if already connected to a server be sure to disconnect first */
    if (get_is_connected_synced()) {
        /* wait for connection thread to exit */
        //TODO: interrupt thread
        void *status;
        pthread_join(input_thread, &status);
    }

    /* start new connection thread */
    set_is_connected_synced(true);
    if (pthread_create(&input_thread, NULL, &threaded_connect, NULL)) {
        errv("Error creating thread\n");
        return 2;
    }
    else {
        debugv("input thread started.\n\n");
    }

    return 0;
}

/* ============== MAIN & INPUT ========================================= */

static char input_buffer[1024+1];

void *process_input()
{
    for(;;) {
        int n = read_line(input_buffer, 1024);
        if (n>0) {
            if (input_buffer[0] == '/') {
                process_command(input_buffer, &execute_command);
            }
            /* check if connected to a server */
            else if(get_is_connected_synced()) {
                datapacket *dp = datapacket_create(MSG_BROADCAST);
                datapacket_set_string(dp, input_buffer);
                send_to_server(dp);
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc >= 2) {
        size_t arg_len = strlen(argv[1]);
        if (arg_len < sizeof(arg_host))
            memcpy(arg_host, argv[1], arg_len);
    }
    if (argc >= 3) {
        arg_port = atoi(argv[2]);
    }

    process_input();

    if (get_is_connected_synced()) {
        //TODO: destroy connection
    }
    
    //TODO: destroy mutex
    pthread_exit(NULL);
    return 0;
}
