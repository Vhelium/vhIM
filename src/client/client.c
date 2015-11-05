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
#include <stdbool.h>

#include "../utility/vstack.h"
#include "../utility/command_parser.h"
#include "../utility/strings_helper.h"

#include "../constants.h"
#include "../network/messagetypes.h"
#include "../network/datapacket.h"

#include "client_ch.h"
#include "client_ui_cons.h"
#include "client_ui_gui.h"
#include "client_callback.h"

#include "../server/server_user.h"

#define PORT 55099
#define HOST "vhelium.com"

static cb_generic_t callbacks[100];

static char arg_host[128] = {0};
static int arg_port;

static int client_connect(const char *server, int port);
static int client_disconnect();

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
            ((cb_welcome_t)callbacks[MSG_WELCOME])(msg);
            free(msg);
        }
        break;

        case MSG_AUTH_FAILED:  {
            char *msg = datapacket_get_string(dp);
            int res = datapacket_get_int(dp);
            ((cb_auth_failed_t)callbacks[MSG_AUTH_FAILED])(msg, res);
            free(msg);
        }
        break;

        case MSG_BROADCAST: {
            char *name = datapacket_get_string(dp);
            char *msg = datapacket_get_string(dp);
            ((cb_broadcast_t)callbacks[MSG_BROADCAST])(name, msg);
            free(name);
            free(msg);
        }
        break;

        case MSG_SYSTEM_MSG: {
            char *msg = datapacket_get_string(dp);
            ((cb_system_msg_t)callbacks[MSG_SYSTEM_MSG])(msg);
            free(msg);
        }
        break;

        case MSG_REGISTR_SUCCESSFUL: {
            char *msg = datapacket_get_string(dp);
            ((cb_registr_successful_t)callbacks[MSG_REGISTR_SUCCESSFUL])(msg);
            free(msg);
        }

        case MSG_REGISTR_FAILED: {
            char *msg = datapacket_get_string(dp);
            ((cb_registr_failed_t)callbacks[MSG_REGISTR_FAILED])(msg);
            free(msg);
        }
        break;

        case MSG_WHO: {
            struct vstack *users = vstack_create();
            int i, count = datapacket_get_int(dp);

            for (i=0; i<count; ++i) {
                int uid = datapacket_get_int(dp);
                char *uname = datapacket_get_string(dp);

                /* save on the stack */
                struct server_user_info *info = malloc(sizeof(struct server_user_info));
                info->username = malloc(strlen(uname) + 1);
                strcpy(info->username, uname);
                info->id = uid;

                vstack_push(users, info);

                free(uname);
            }

            /* callback has to make sure to free the info structs, leaving an empty stack! */
            ((cb_who_t)callbacks[MSG_WHO])(users);

            /* make sure the stack is empty */
            while (!vstack_is_empty(users)) {
                struct server_user_info *info = (struct server_user_info *)vstack_pop(users);
                free(info->username);
                free(info);
            }
            /* free stack struct */
            free(users);
        }
        break;

        case MSG_FRIENDS: {
            struct vstack *on = vstack_create(), *off = vstack_create(), *req = vstack_create();
            int i, req_len, off_len, on_len = datapacket_get_int(dp);

            /* online */
            for (i=0; i<on_len; ++i) {
                int uid = datapacket_get_int(dp);
                char *u = datapacket_get_string(dp);

                /* save on the stack */
                struct server_user_info *info = malloc(sizeof(struct server_user_info));
                info->username = malloc(strlen(u) + 1);
                strcpy(info->username, u);
                info->id = uid;

                vstack_push(on, info);

                free(u);
            }

            /* offline */
            off_len = datapacket_get_int(dp);
            for (i=0; i<off_len; ++i) {
                int uid = datapacket_get_int(dp);
                char *u = datapacket_get_string(dp);

                /* save on the stack */
                struct server_user_info *info = malloc(sizeof(struct server_user_info));
                info->username = malloc(strlen(u) + 1);
                strcpy(info->username, u);
                info->id = uid;

                vstack_push(off, info);

                free(u);
            }

            /* requests */
            req_len = datapacket_get_int(dp);
            for (i=0; i<req_len; ++i) {
                int uid = datapacket_get_int(dp);
                char *u = datapacket_get_string(dp);

                /* save on the stack */
                struct server_user_info *info = malloc(sizeof(struct server_user_info));
                info->username = malloc(strlen(u) + 1);
                strcpy(info->username, u);
                info->id = uid;

                vstack_push(req, info);

                free(u);
            }

            /* callback has to make sure to free the info structs, leaving empty stacks */
            ((cb_friends_t)callbacks[MSG_FRIENDS])(on, off, req);

            /* make sure the stack is empty */
            while (!vstack_is_empty(on)) {
                struct server_user_info *info = (struct server_user_info *)vstack_pop(on);
                free(info->username);
                free(info);
            }
            while (!vstack_is_empty(off)) {
                struct server_user_info *info = (struct server_user_info *)vstack_pop(off);
                free(info->username);
                free(info);
            }
            while (!vstack_is_empty(req)) {
                struct server_user_info *info = (struct server_user_info *)vstack_pop(req);
                free(info->username);
                free(info);
            }
            /* free stack structs */
            free(on);
            free(off);
            free(req);
        }
        break;

        case MSG_REMOVE_FRIEND: {
            int uid = datapacket_get_int(dp);
            ((cb_remove_friend_t)callbacks[MSG_REMOVE_FRIEND])(uid);
        }
        break;

        case MSG_FRIEND_ONLINE: {
            int uid = datapacket_get_int(dp);
            char *uname = datapacket_get_string(dp);
            ((cb_friend_online_t)callbacks[MSG_FRIEND_ONLINE])(uid, uname);
            free(uname);
        }
        break;

        case MSG_FRIEND_OFFLINE: {
            int uid = datapacket_get_int(dp);
            ((cb_friend_offline_t)callbacks[MSG_FRIEND_OFFLINE])(uid);
        }
        break;

        default:
            errv("Unknown packet: %d\n", packet_type);
            break;
    }

    datapacket_destroy(dp); // destroy the dp with its data array
}

/* ====================== EXECUTE FUNCTIOMS ================================ */

int cl_exec_broadcast(const char *msg)
{
    datapacket *dp = datapacket_create(MSG_BROADCAST);
    datapacket_set_string(dp, msg);
    send_to_server(dp);
    return 0;
}

int cl_exec_kick(int uid)
{
    datapacket *dp = datapacket_create(MSG_CMD_KICK_ID);
    datapacket_set_int(dp, uid);
    send_to_server(dp);
    return 0;
}

int cl_exec_whisper(int uid, const char *msg)
{
    datapacket *dp = datapacket_create(MSG_WHISPER);
    datapacket_set_int(dp, uid);
    datapacket_set_string(dp, msg);
    send_to_server(dp);
    return 0;
}

int cl_exec_req_register(const char *name, const char *pw)
{
    datapacket *dp = datapacket_create(MSG_REQ_REGISTER);
    datapacket_set_string(dp, name);
    datapacket_set_string(dp, pw);
    send_to_server(dp);
    return 0;
}

int cl_exec_req_login(const char *name, const char *pw)
{
    datapacket *dp = datapacket_create(MSG_REQ_LOGIN);
    datapacket_set_string(dp, name);
    datapacket_set_string(dp, pw);
    send_to_server(dp);
    return 0;
}

int cl_exec_logout(void)
{
    datapacket *dp = datapacket_create(MSG_LOGOUT);
    send_to_server(dp);
    return 0;
}

int cl_exec_who(void)
{
    datapacket *dp = datapacket_create(MSG_WHO);
    send_to_server(dp);
    return 0;
}

int cl_exec_friends(void)
{
    datapacket *dp = datapacket_create(MSG_FRIENDS);
    send_to_server(dp);
    return 0;
}

int cl_exec_connect(const char *server, int port)
{
    return client_connect(server, port);
}

int cl_exec_disconnect(void)
{
    client_disconnect();
    return 0;
}

int cl_exec_grant_privileges(int uid, int lvl)
{
    datapacket *dp = datapacket_create(MSG_GRANT_PRIVILEGES);
    datapacket_set_int(dp, uid);
    datapacket_set_int(dp, lvl);
    send_to_server(dp);
    return 0;
}

int cl_exec_add_friend(int uid)
{
    datapacket *dp = datapacket_create(MSG_ADD_FRIEND);
    datapacket_set_int(dp, uid);
    send_to_server(dp);
    return 0;
}

int cl_exec_remove_friend(int uid)
{
    datapacket *dp = datapacket_create(MSG_REMOVE_FRIEND);
    datapacket_set_int(dp, uid);
    send_to_server(dp);
    return 0;
}

int cl_exec_group_create(const char *name)
{
    datapacket *dp = datapacket_create(MSG_GROUP_CREATE);
    datapacket_set_string(dp, name);
    send_to_server(dp);
    return 0;
}

int cl_exec_group_delete(int uid)
{
    //TODO
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

bool cl_get_is_connected_synced()
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

    client_ch_listen(&process_packet);

    client_ch_destroy();
    printf("disconnected.\n");

    set_is_connected_synced(false);

    return NULL;
}

static int client_connect(const char *host, int port)
{
    if (!host) { /* no direct argument passed */
        if(arg_host[0] != 0) { /* argument was passed at startup */
            /* do nothing as it's already at the right place :) */
        }
        else { /* no arguments. Use default host */
            memcpy(arg_host, HOST, strlen(HOST)+1);
        }
    }
    else { /* direct argument passed */
        memcpy(arg_host, host, strlen(host)+1);
    }
    if (!port) {
        arg_port = PORT;
    }
    else {
        arg_port = port;
    }

    client_disconnect();

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

static int client_disconnect()
{
    /* if already connected to a server be sure to disconnect first */
    if (cl_get_is_connected_synced()) {
        /* send msg to server */
        datapacket *msg = datapacket_create(MSG_DISCONNECT);
        send_to_server(msg);

        /* close socket */
        client_ch_destroy();
        set_is_connected_synced(false);
        /* wait for listener thread to quit */
        void *status;
        pthread_join(input_thread, &status);
    }
    return 0;
}

/* ============== MAIN ========================================= */

int main(int argc, char *argv[])
{
    bool is_gui = false;
    int i, param_count = 0;
    int ret = 0;
    for(i = 1; i < argc; ++i) {
        if (argv[i][0] == '-' && argv[i][1] == '-') { /* modifier */
            if(strcmp(&argv[i][2], "gui") == 0) { /* gui */
                is_gui = true;
            }
        }
        else if (param_count == 0) { /* server */
            size_t arg_len = strlen(argv[i]);
            if (arg_len < sizeof(arg_host))
                memcpy(arg_host, argv[i], arg_len+1);

            param_count++;
        }
        else if (param_count == 1) { /* port */
            arg_port = atoi(argv[i]);

            param_count++;
        }
    }

    if (is_gui) {
        ret = cl_ui_gui_start(callbacks, PORT);
    }
    else {
        cl_ui_cons_start(callbacks, PORT);
    }

    /* destroy the connection if it is still active. */
    if (cl_get_is_connected_synced()) {
        client_ch_destroy();
    }
    
    pthread_mutex_destroy(&mutex_connected);
    return ret;
}
