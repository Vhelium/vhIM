#include "client_ui_cons.h"
#include "client_callback.h"
#include "../network/messagetypes.h"

#include "../utility/vstack.h"
#include "../utility/command_parser.h"
#include "../utility/strings_helper.h"

#include "../constants.h"

#include "client.h"
#include "client_callback.h"

#include "../server/server_user.h"

static int port_default;

static void cb_welcome (const char *msg)
{
    printf("[INFO]: Server welcomes you: %s\n", msg);
}

static void cb_auth_failed(const char *msg, int res)
{
    printf("[INFO]: %s\nError Code: %d\n", msg, res);
}

static void cb_broadcast(const char *name, const char *msg)
{
    printf("[%s]: %s\n", name, msg);
}

static void cb_system_msg(const char *msg)
{
    printf("[SERVER]: %s\n", msg);
}

static void cb_registr_successful(const char *msg)
{
    printf("%s\n", msg);
}

static void cb_registr_failed(const char *msg)
{
    printf("[server]: %s\n", msg);
}

static void cb_who(struct vstack *users)
{
    printf("Users online: %lu\n", vstack_get_size(users));

    while (!vstack_is_empty(users)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(users);
        printf("    %s(%d)\n", info->username, info->id);
        free(info->username);
        free(info);
    }
    printf("\n");

}

static void cb_friends(struct vstack *on, struct vstack *off, struct vstack *req)
{
    printf("Friends online: %lu\n", vstack_get_size(on));
    while (!vstack_is_empty(on)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(on);
        printf("    %s(%d)\n", info->username, info->id);
        free(info->username);
        free(info);
    }
    printf("\n");

    printf("Friends offline: %lu\n", vstack_get_size(off));
    while (!vstack_is_empty(off)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(off);
        printf("    %s(%d)\n", info->username, info->id);
        free(info->username);
        free(info);
    }
    printf("\n");

    printf("Pending friend requests: %lu\n", vstack_get_size(req));
    while (!vstack_is_empty(req)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(req);
        printf("    %s(%d)\n", info->username, info->id);
        free(info->username);
        free(info);
    }
    printf("\n");
}

static void cb_remove_friend(int uid)
{
    printf("Friend removed: %d\n", uid);
}

static void cb_friend_online(int uid, const char *name)
{
    printf("Friend came online: %s(%d)\n", name, uid);
}

static void cb_friend_offline(int uid)
{
    printf("Friend with id %d went offline.\n", uid);
}

/* ========================================================================= */

static int execute_command(int type, char *argv[])
{
    switch (type)
    {
        case MSG_CMD_KICK_ID: {
            /* check if passed argument is a number */
            if (!is_decimal_number(argv[0]))
                return 3;
            int uid = atoi(argv[0]);

            cl_exec_kick(uid);
        }
        break;

        case MSG_WHISPER: {
            /* check if passed argument is a number */
            if (!is_decimal_number(argv[0]))
                return 3;
            int uid = atoi(argv[0]);

            cl_exec_whisper(uid, argv[1]);
        }
        break;

        case MSG_REQ_REGISTER: {
            cl_exec_req_register(argv[0], argv[1]);
        }
        break;

        case MSG_REQ_LOGIN: {
            cl_exec_req_login(argv[0], argv[1]);
        }
        break;

        case MSG_LOGOUT: {
            cl_exec_logout();
        }
        break;

        case MSG_WHO: {
            cl_exec_who();
        }
        break;

        case MSG_FRIENDS: {
            cl_exec_friends();
        }
        break;

        case CMD_CONNECT: {
            /* check if passed argument is a number */
            if (argv[1] != NULL && !is_decimal_number(argv[1]))
                return 3;
            int port = argv[1] != NULL ? atoi(argv[1]) : port_default;
            if (cl_exec_connect(argv[0], port)) {
                printf("error connecting to host\n");
                exit(1);
            }
        }
        break;

        case CMD_DISCONNECT: {
            cl_exec_disconnect();
        }
        break;

        case MSG_GRANT_PRIVILEGES: {
            if (is_decimal_number(argv[0]) && is_decimal_number(argv[1])) {
                cl_exec_grant_privileges(atoi(argv[0]), atoi(argv[1]));
            }
            else
                printf("invalid arguments.\n");
        }
        break;

        case MSG_ADD_FRIEND: {
            if (is_decimal_number(argv[0])) {
                cl_exec_add_friend(atoi(argv[0]));
            }
        }
        break;
 
        case MSG_REMOVE_FRIEND: {
            if (is_decimal_number(argv[0])) {
                cl_exec_remove_friend(atoi(argv[0]));
            }
        }
        break;

        case MSG_GROUP_CREATE: {
            cl_exec_group_create(argv[0]);
        }
        break;

        case MSG_GROUP_DELETE: {
            if (is_decimal_number(argv[0])) {
                cl_exec_group_delete(atoi(argv[0]));
            }
        }
        break;

        case CMD_HELP: {
            printf("Go fuck yourself.\n");
        }
        break;

        default:
            printf("Invalid command\n");
            return 1;
    }

    return 0;
}

/* ========================================================================= */

static char input_buffer[1024+1];

static void process_input()
{
    for(;;) {
        int n = read_line(input_buffer, 1024);
        if (n>0) {
            if (input_buffer[0] == '/') {
                process_command(input_buffer, &execute_command);
            }
            /* check if connected to a server */
            else if(cl_get_is_connected_synced()) {
                cl_exec_broadcast(input_buffer);
            }
            else
                printf("Not connected to a server.\n");
        }
    }
}

void cl_ui_cons_start(cb_generic_t *cbs, int port)
{
    port_default = port;

    /* initialize callbacks: */
    cbs[MSG_WELCOME] = (cb_generic_t)&cb_welcome;
    cbs[MSG_AUTH_FAILED] = (cb_generic_t)&cb_auth_failed;
    cbs[MSG_BROADCAST] = (cb_generic_t)&cb_broadcast;
    cbs[MSG_SYSTEM_MSG] = (cb_generic_t)&cb_system_msg;
    cbs[MSG_REGISTR_SUCCESSFUL] = (cb_generic_t)&cb_registr_successful;
    cbs[MSG_REGISTR_FAILED] = (cb_generic_t)&cb_registr_failed;
    cbs[MSG_WHO] = (cb_generic_t)&cb_who;
    cbs[MSG_FRIENDS] = (cb_generic_t)&cb_friends;
    cbs[MSG_REMOVE_FRIEND] = (cb_generic_t)&cb_remove_friend;
    cbs[MSG_FRIEND_ONLINE] = (cb_generic_t)&cb_friend_online;
    cbs[MSG_FRIEND_OFFLINE] = (cb_generic_t)&cb_friend_offline;

    /* initialize UI and start input thread */
    process_input();
}
