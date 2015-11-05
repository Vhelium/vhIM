#include <unistd.h>

#include "client_ui_gui.h"
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
}

static void cb_auth_failed(const char *msg, int res)
{
}

static void cb_broadcast(const char *name, const char *msg)
{
}

static void cb_system_msg(const char *msg)
{
}

static void cb_registr_successful(const char *msg)
{
}

static void cb_registr_failed(const char *msg)
{
}

static void cb_who(struct vstack *users)
{
    while (!vstack_is_empty(users)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(users);
        /*
         * TODO:
         * info->username
         * info->id
         */
        free(info->username);
        free(info);
    }
}

static void cb_friends(struct vstack *on, struct vstack *off, struct vstack *req)
{
    /* online */
    while (!vstack_is_empty(on)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(on);
        /*
         * TODO:
         * info->username
         * info->id
         */
        free(info->username);
        free(info);
    }

    /* offline */
    while (!vstack_is_empty(off)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(off);
        /*
         * TODO:
         * info->username
         * info->id
         */
        free(info->username);
        free(info);
    }

    /* requests */
    while (!vstack_is_empty(req)) {
        struct server_user_info *info = (struct server_user_info *)vstack_pop(req);
        /*
         * TODO:
         * info->username
         * info->id
         */
        free(info->username);
        free(info);
    }
}

static void cb_remove_friend(int uid)
{
}

static void cb_friend_online(int uid, const char *name)
{
}

static void cb_friend_offline(int uid)
{
}

/* =========================== INITIALIZATION ============================== */

int cl_ui_gui_start(cb_generic_t *cbs, int port)
{
    port_default = port;

    // TODO: initialize callbacks
    cbs[MSG_WELCOME] = (cb_generic_t)NULL;
    cbs[MSG_AUTH_FAILED] = (cb_generic_t)NULL;
    cbs[MSG_BROADCAST] = (cb_generic_t)NULL;
    cbs[MSG_SYSTEM_MSG] = (cb_generic_t)NULL;
    cbs[MSG_REGISTR_SUCCESSFUL] = (cb_generic_t)NULL;
    cbs[MSG_REGISTR_FAILED] = (cb_generic_t)NULL;
    cbs[MSG_WHO] = (cb_generic_t)NULL;
    cbs[MSG_FRIENDS] = (cb_generic_t)NULL;
    cbs[MSG_REMOVE_FRIEND] = (cb_generic_t)NULL;
    cbs[MSG_FRIEND_ONLINE] = (cb_generic_t)NULL;
    cbs[MSG_FRIEND_OFFLINE] = (cb_generic_t)NULL;

    //TODO: initialize UI and start input thread
    printf("Starting GUI..\n");
    sleep(1);
    printf("just kidding!\n");
    exit(0);

    return 0;
}
