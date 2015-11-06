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

/* =========================== GUI-SETUP ============================== */

gboolean init_app(ClientGuiApp *app)
{
    GtkBuilder *builder;
    GError *err = NULL;

    builder = gtk_builder_new();
    // Load the GUI from the .ui file, exiting on error.
    if(gtk_builder_add_from_resource(builder, "/res/client_ui_gui_main.ui", &err) == 0){
        // TODO: print error.
        g_error_free(err);
        return FALSE;
    }

    app->main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    app->menu_bar = GTK_WIDGET(gtk_builder_get_object(builder, "menu_bar"));
    app->status_bar = GTK_WIDGET(gtk_builder_get_object(builder, "status_bar"));

    // Clean up after using the builder.
    gtk_builder_connect_signals(builder, app);
    g_object_unref(G_OBJECT(builder));
    
    // Set the status bar context id.
    app->status_bar_context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(
                app->status_bar), "vhIM client (gui running)");

    return TRUE; 
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

    // Initialize the GUI.
    ClientGuiApp *client;
    client = g_slice_new(ClientGuiApp);
    // Initialize the gtk environment. No arguments are getting passed.
    gtk_init(NULL, NULL);
    // Try to initialize the application, return 1 on failure.
    if(init_app(client) == FALSE) return 1;
    gtk_widget_show(client->main_window);
    // Main GTK-Appliacation loop.
    gtk_main();
    g_slice_free(ClientGuiApp, client); // Free residual memory.
    return 0; // Successfull execution.
}
