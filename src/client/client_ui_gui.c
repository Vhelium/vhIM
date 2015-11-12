#include <unistd.h>

#include <gdk/gdkkeysyms.h>

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
static ClientGuiApp *client;
static GtkTextBuffer *out_buff;

static void cb_exit(void)
{
    gui_quit_request();
}

static void cb_welcome (const char *msg)
{
    char print_msg[50];
    sprintf(print_msg, "Welcome, %.35s!\n", msg);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_SERVER);
}

static void cb_auth_failed(const char *msg, int res)
{
    char print_msg[100];
    sprintf(print_msg, "%.50s\nError Code: %d\n", msg, res);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_ERROR);
}

static void cb_broadcast(const char *name, const char *msg)
{
    char print_msg[strlen(msg) + 3];
    sprintf(print_msg, "%s\n", msg);
    gui_print(print_msg, name, GUI_MSG_TYPE_USER);
}

static void cb_system_msg(const char *msg)
{
    char print_msg[strlen(msg) + 3];
    sprintf(print_msg, "%s\n", msg);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_SERVER);
}

static void cb_registr_successful(const char *msg)
{
    char print_msg[strlen(msg) + 3];
    sprintf(print_msg, "%s\n", msg);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_SERVER);
}

static void cb_registr_failed(const char *msg)
{
    char print_msg[strlen(msg) + 3];
    sprintf(print_msg, "%s\n", msg);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_SERVER);
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

// TODO: Store and handle names.
static void cb_remove_friend(int uid)
{
    char print_msg[50];
    sprintf(print_msg, "Friend with id %d has been removed.\n", uid);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_INFO);
}

static void cb_friend_online(int uid, const char *name)
{
    char print_msg[100];
    sprintf(print_msg, "Your friend, %.35s [#%d] came online.\n", name, uid);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_INFO);
}

static void cb_friend_offline(int uid)
{
    char print_msg[50];
    sprintf(print_msg, "Friend with id %d has gone offline.\n", uid);
    gui_print(print_msg, NULL, GUI_MSG_TYPE_INFO);
}

static void cb_group_dump_active(const char *msg)
{
    // TODO
}

static void cb_group_delete(int gid)
{
    // TODO
}

static void cb_group_owner_changed(int gid, int uid)
{
    // TODO
}

static void cb_txt_group(int git, int uid, const char *msg)
{
    // TODO
}

static void cb_client_disconnected(void)
{
    gui_print("Disconnected!\n", NULL, GUI_MSG_TYPE_INFO);
}

static void cb_client_connected(void)
{
    gui_print("Connected!\n", NULL, GUI_MSG_TYPE_INFO);
}

static void cb_client_destroyed(void)
{
    // TODO: Possible cleanup code?...
}

/* ==================================================================== */

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
        case CMD_EXIT:
            cb_exit();
            break;
        case CMD_CONNECT:
            if(argv[1] != NULL && !is_decimal_number(argv[1])) return 3;
            int port = argv[1] != NULL ? atoi(argv[1]) : port_default;
            if(cl_exec_connect(argv[0], port)){
                gui_print("Error connecting to host.\n"
                        , NULL, GUI_MSG_TYPE_ERROR);
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
                gui_print("Error, invalid arguments.\n"
                        , NULL, GUI_MSG_TYPE_ERROR);
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
                gui_print("Invalid arguments.\n"
                        , NULL, GUI_MSG_TYPE_ERROR);
            break;
        case MSG_GROUP_REMOVE_USER:
            if(is_decimal_number(argv[0]) && is_decimal_number(argv[1]))
                cl_exec_group_remove_user(atoi(argv[0]), atoi(argv[1]));
            else
                gui_print("Invalid arguments.\n"
                        , NULL, GUI_MSG_TYPE_ERROR);
            break;
        case MSG_TXT_GROUP:
            if(!is_decimal_number(argv[0]))
                return 3;
            cl_exec_group_send(atoi(argv[0]), argv[1]);
            break;
        case MSG_DUMP_ACTIVE_GROUPS:
            cl_exec_group_dump_active();
            break;
        case MSG_GROUP_WHO:
            if(is_decimal_number(argv[0]))
                cl_exec_group_who(atoi(argv[0]));
            break;
        case CMD_HELP:
            // TODO: Handle help command.
            gui_print("Go fuck yourself.\n"
                    , NULL, GUI_MSG_TYPE_INFO);
            break;
        default:
            gui_print("Invalid command.\n"
                    , NULL, GUI_MSG_TYPE_ERROR);
            return 1;
    }
    return 0;
}

static void process_input(char *input, int length)
{
    if(length > 0){
        if(cl_get_is_connected_synced()){
            if(input[0] == '/'){
                process_command(input, &execute_command);
            }else{
                cl_exec_broadcast(input);
                gui_print(strcat(input, "\n"), "You", GUI_MSG_TYPE_BROADCAST);
            }
        }else{
            if(!process_offline_command(input, &execute_command))
                gui_print("Not connected to a server.\n"
                        , NULL, GUI_MSG_TYPE_ERROR);
        }
    }
}

/* =========================== GUI-HANDLERS =========================== */

void gui_quit_request(void)
{
    // TODO: Ask for user confirmation before quiting. 
    gtk_main_quit();
}

void gui_print(const char *msg, const char *origin, int type)
{
    // Check if the message exists.
    if(!msg) return;
    
    // Grab the current local time.
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // Grab the output buffer and set end iterator.
    out_buff = gtk_text_view_get_buffer(client->output);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(out_buff, &end);

    // Print the time stamp in front of the message.
    char time_stamp[8];
    sprintf(time_stamp, "[%.2d:%.2d]", tm.tm_hour, tm.tm_min);
    gtk_text_buffer_insert_with_tags_by_name(out_buff, &end, time_stamp, -1,
            "weight_normal", "color_blue", NULL);

    // Add origin tag to the message.
    if(type == GUI_MSG_TYPE_USER ||
            type == GUI_MSG_TYPE_BROADCAST){
        // Check if the origin exists.
        if(!origin) return;
        // TODO: Use user specific color (Session dependent).
        char print_msg[50];
        sprintf(print_msg, "[%.40s]>>\t", origin);
        if(type == GUI_MSG_TYPE_USER)
            gtk_text_buffer_insert_with_tags_by_name(
                    out_buff, &end, print_msg, -1,
                    "weight_bold", "color_black", NULL);
        else
            gtk_text_buffer_insert_with_tags_by_name(
                    out_buff, &end, print_msg, -1,
                    "weight_bold", "color_orange", NULL);
    }else{
        switch(type){
            case GUI_MSG_TYPE_INFO:
                gtk_text_buffer_insert_with_tags_by_name(
                        out_buff, &end, "[INFO]>>\t", -1,
                        "weight_bold", "color_green", NULL);
                break;
            case GUI_MSG_TYPE_SERVER:
                gtk_text_buffer_insert_with_tags_by_name(
                        out_buff, &end, "[SERVER]>>\t", -1,
                        "weight_bold", "color_blue", NULL);
                break;
            case GUI_MSG_TYPE_ERROR:
                gtk_text_buffer_insert_with_tags_by_name(
                        out_buff, &end, "[ERROR]>>\t", -1,
                        "weight_bold", "color_red", NULL);
                break;
            default:
                // TODO: Handle?
                g_print("IO Error! >> Code 0x001\n");
                break;
        }
    }

    // Update end iterator and print message.
    gtk_text_buffer_insert_with_tags_by_name(out_buff, &end, msg, -1,
            "weight_normal", "color_black", NULL);

    // Write the buffer back to the text view.
    gtk_text_view_set_buffer(client->output, out_buff);

    // Insert a mark to the end of the text and scroll to it.
    GtkTextMark *mark = gtk_text_mark_new("mrk", TRUE);
    gtk_text_buffer_add_mark(out_buff, mark, &end);
    gtk_text_view_scroll_to_mark(client->output, mark, 0.0, FALSE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(out_buff, mark);
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *pKey, ClientGuiApp *app)
{
    // Check if the enter key was pressed.
    // If yes, do not propagate the key event (block it by returning true).
    if(pKey->type == GDK_KEY_PRESS && pKey->keyval == GDK_KEY_Return){
        send_button_clicked(NULL, app);
        return TRUE;
    }
    // The key pressed was not enter. Propagate it.
    return GDK_EVENT_PROPAGATE;
}

// TODO: Hinder sending on empty input field.
void send_button_clicked(GtkWidget *button, ClientGuiApp *app)
{
    // Read the text from the input field.
    GtkTextIter start, end;
    GtkTextBuffer *buff = gtk_text_view_get_buffer(app->input);
    gtk_text_buffer_get_bounds(buff, &start, &end);
    gchar *input = gtk_text_buffer_get_text(buff, &start, &end, FALSE);
    process_input(input, gtk_text_iter_get_offset(&end) + 1);

    // Clear the input text field.
    gtk_text_buffer_set_text(buff, "", -1);
    gtk_text_view_set_buffer(app->input, buff);
}

/* =========================== GUI-SETUP ============================== */

void set_up_output_buffer(){
    out_buff = gtk_text_buffer_new(NULL);
    GdkRGBA color;

    gdk_rgba_parse(&color, "orange");
    gtk_text_buffer_create_tag(out_buff, "color_orange", "foreground-rgba", &color, NULL);
    gdk_rgba_parse(&color, "red");
    gtk_text_buffer_create_tag(out_buff, "color_red", "foreground-rgba", &color, NULL);
    gdk_rgba_parse(&color, "black");
    gtk_text_buffer_create_tag(out_buff, "color_black", "foreground-rgba", &color, NULL);
    gdk_rgba_parse(&color, "blue");
    gtk_text_buffer_create_tag(out_buff, "color_blue", "foreground-rgba", &color, NULL);
    gdk_rgba_parse(&color, "green");
    gtk_text_buffer_create_tag(out_buff, "color_green", "foreground-rgba", &color, NULL);
    gtk_text_buffer_create_tag(out_buff, "weight_normal", "weight", PANGO_WEIGHT_NORMAL, NULL);
    gtk_text_buffer_create_tag(out_buff, "weight_bold", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_buffer_create_tag(out_buff, "weight_thin", "weight", PANGO_WEIGHT_THIN, NULL);

    gtk_text_view_set_buffer(client->output, out_buff);
}

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
    // Make input react to enter key press.
    g_signal_connect(app->input, "key_press_event", G_CALLBACK(on_key_press), app);

    // Set up the colors and styles for the output text window.
    set_up_output_buffer();

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

    // Initialize callbacks
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
    cbs[MSG_DUMP_ACTIVE_GROUPS] = (cb_generic_t)&cb_group_dump_active;
    cbs[MSG_GROUP_DELETE] = (cb_generic_t)&cb_group_delete;
    cbs[MSG_GROUP_OWNER_CHANGED] = (cb_generic_t)&cb_group_owner_changed;
    cbs[MSG_TXT_GROUP] = (cb_generic_t)&cb_txt_group;
    cbs[CB_CLIENT_DISCONNECTED] = (cb_generic_t)&cb_client_disconnected;
    cbs[CB_CLIENT_CONNECTED] = (cb_generic_t)&cb_client_connected;
    cbs[CB_CLIENT_DESTROYED] = (cb_generic_t)&cb_client_destroyed;

    // Initialize the GUI.
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
