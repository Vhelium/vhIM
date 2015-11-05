#include <unistd.h>

#include "client_ui_gui.h"
#include "client_ui_gui_window.h"
#include "client_callback.h"
#include "../network/messagetypes.h"

#include "../utility/vstack.h"
#include "../utility/command_parser.h"
#include "../utility/strings_helper.h"

#include "../constants.h"

#include "client.h"
#include "client_callback.h"

#include "../server/server_user.h"

G_DEFINE_TYPE(ClientGui, cl_ui_gui, GTK_TYPE_APPLICATION);

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

/* =========================== GUI-SETUP ============================== */

static void cl_ui_gui_init(ClientGui *app)
{
}

static void cl_ui_gui_activate(GApplication *app)
{
    ClientGuiWindow *window;
    window = cl_ui_gui_window_new(CLIENT_UI_GUI(app));
    gtk_window_present(GTK_WINDOW(window));
}

static void cl_ui_gui_open(GApplication *app, GFile **files
        , gint n_files, const gchar *hint)
{
    ClientGuiWindow *win;
    GList *windows = gtk_application_get_windows(GTK_APPLICATION(app));

    if (windows)
        win = CLIENT_UI_GUI_WINDOW(windows->data);
    else
        win = cl_ui_gui_window_new(CLIENT_UI_GUI(app));

    for (int i = 0; i < n_files; i++)
        cl_ui_gui_window_open(win, files[i]);

    gtk_window_present(GTK_WINDOW(win));
}

static void cl_ui_gui_class_init(ClientGuiClass *class)
{
    G_APPLICATION_CLASS(class)->activate = cl_ui_gui_activate;
    G_APPLICATION_CLASS(class)->open = cl_ui_gui_open;
}

ClientGui * cl_ui_gui_new()
{
    return g_object_new(CLIENT_UI_GUI_TYPE, "application-id"
            , "com.vhelium.vhIM", "flags"
            , G_APPLICATION_HANDLES_OPEN, NULL);
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
    return g_application_run(G_APPLICATION(cl_ui_gui_new()), 0, NULL);
}
