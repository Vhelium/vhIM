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

static int execute_command(int type, char *argv[])
{
    switch(type){
        case MSG_CMD_KICK_ID:
            if(!is_decimal_number(argv[0])) return 3;
            cl_exec_kick(atoi(argv[0]));
            break;
        case MSG_WHISPER:
            if(!is_decimal_number(argv[0])) return 3;
            cl_exec_whisper(atoi(argv[0]), argv[1]);
            break;
        case MSG_REQ_REGISTER:
            cl_exec_req_register(argv[0], argv[1]);
            break;
        case MSG_REQ_LOGIN:
            cl_exec_req_login(argv[0], argv[1]);
            break;
        case MSG_LOGOUT:
            cl_exec_logout();
            break;
        case MSG_WHO:
            cl_exec_who();
            break;
        case MSG_FRIENDS:
            cl_exec_friends();
            break;
        case CMD_CONNECT:
            if(argv[1] != NULL && !is_decimal_number(argv[1])) return 3;
            int port = argv[1] != NULL ? atoi(argv[1]) : port_default;
            if(cl_exec_connect(argv[0], port)){
                // TODO: Print error connecting to host.
                g_print("Error connecting to host.\n");
                exit(1);
            }
            break;
        case CMD_DISCONNECT:
            cl_exec_disconnect();
            break;
        case MSG_GRANT_PRIVILEGES:
            if(is_decimal_number(argv[0]) && is_decimal_number(argv[1]))
                cl_exec_grant_privileges(atoi(argv[0]), atoi(argv[1]));
            else
                // TODO: Print error invalid arguments
                g_print("Error, invalid arguments.\n");
            break;
        case MSG_ADD_FRIEND:
            if(is_decimal_number(argv[0])) cl_exec_add_friend(atoi(argv[0]));
            break;
        case MSG_REMOVE_FRIEND:
            if(is_decimal_number(argv[0])) cl_exec_remove_friend(atoi(argv[0]));
            break;
        case MSG_GROUP_CREATE:
            cl_exec_group_create(argv[0]);
            break;
        case MSG_GROUP_DELETE:
            if(is_decimal_number(argv[0])) cl_exec_group_delete(atoi(argv[0]));
            break;
        case MSG_GROUP_ADD_USER:
            if(is_decimal_number(argv[0]) && is_decimal_number(argv[1]))
                cl_exec_group_add_user(atoi(argv[0]), atoi(argv[1]));
            else
                // TODO: Print error invalid arguments.
                g_print("Error, invalid arguments.\n");
            break;
        case CMD_HELP:
            // TODO: Handle help command.
            g_print("Go fuck yourself.\n");
            break;
        default:
            // TODO: Handle invalid command.
            g_print("Invalid command.\n");
            return 1;
    }
    return 0;
}

static void process_input(char *input, int length)
{
    if(length > 0){
        if(input[0] == '/'){
            process_command(input, &execute_command);
        }else if(cl_get_is_connected_synced()){
            cl_exec_broadcast(input);
        }else{
            // TODO: Return error: Not connected to server
        }
    }
}

/* =========================== GUI-HANDLERS =========================== */

void send_button_clicked(GtkWidget *button, ClientGuiApp *app){
    GtkTextIter start, end;
    GtkTextBuffer *buff = gtk_text_view_get_buffer(app->input);
    gtk_text_buffer_get_bounds(buff, &start, &end);
    process_input(gtk_text_buffer_get_text(buff, &start, &end, FALSE), gtk_text_iter_get_offset(&end) + 1);

    // Clear the input text field.
    gtk_text_buffer_set_text(buff, "", -1);
    gtk_text_view_set_buffer(app->input, buff);
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

    // Grab and assign all necessary gtk widgets.
    app->main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    app->menu_bar = GTK_WIDGET(gtk_builder_get_object(builder, "menu_bar"));
    app->output = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "output_text_view"));
    app->input = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "input_text_view"));
    app->send_button = GTK_BUTTON(gtk_builder_get_object(builder, "input_send_button"));
    app->status_bar = GTK_WIDGET(gtk_builder_get_object(builder, "status_bar"));

    // --------- Set action handlers ----------
    // Input-Send-Button:
    g_signal_connect(app->send_button, "clicked", G_CALLBACK(send_button_clicked), app);

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
