#ifndef CLIENT_UI_GUI_H
#define CLIENT_UI_GUI_H

#include "client_callback.h"

#include <gtk/gtk.h>

#define GUI_MSG_TYPE_INFO 0
#define GUI_MSG_TYPE_USER 1
#define GUI_MSG_TYPE_SERVER 2
#define GUI_MSG_TYPE_ERROR 3

/* ================== Structs ================== */

/*
 * Struct: ClientGuiApp
 * --------------------
 * Description:
 * The basic application type.
 */
typedef struct ClientGuiApp {
    GtkWidget *main_window, *menu_bar, *status_bar;
    GtkTextView *output, *input; 
    GtkButton *send_button;
    guint status_bar_context_id;
} ClientGuiApp;

/* ================== Functions ================ */

/*
 * Function: gui_print
 * -------------------
 * Description:
 * Print a standard text output to the gui's
 * output text field.
 *
 * Arguments: msg - The message to print.
 *            origin - The sender of the message.
 *            type - The message type.
 *
 * Returns: void
 */
void gui_print(const char *msg, const char *origin, int type);

/*
 * Function: send_button_clicked
 * -----------------------------
 * Description:
 * Generate the send-event. This reads the input from
 * the input text field and processes it, before
 * clearing out the input field.
 *
 * Arguments: button - Pointer to the pressed button.
 *            app - Pointer to the application.
 *
 * Returns: void
 */
void send_button_clicked(GtkWidget *button, ClientGuiApp *app);

/*
 * Function: on_key_press
 * ----------------------
 * Description:
 * Check if a pressed key (in a text view) is the 
 * enter key or not. If it is the enter key, do not
 * propagate the input, and instead call the
 * send_button_clicked method to process the input.
 *
 * Arguments: widget - The text field.
 *            pkey - The key press-event.
 *            app - Pointer to the application.
 *
 * Returns: TRUE - Do not propagate the event.
 *          GDK_EVENT_PROPAGATE - Propagate the event.
 */
gboolean on_key_press(GtkWidget *widget, GdkEventKey *pkey, ClientGuiApp *app);

/*
 * Function: init_app
 * ------------------
 * Description:
 * Sets everything up for the client.
 * Handles the basic GUI setup and assigns
 * all necessary handling functions etc.
 *
 * Arguments: app - Pointer to the app to set up (this)
 *
 * Returns: a boolean true on success, and false on failure.
 */
gboolean init_app(ClientGuiApp *app);

/*
 * Function: cl_ui_gui_start
 * -------------------------
 * Description:
 * Initializes all the callbacks and starts the GUI
 * Environment for the client.
 *
 * Arguments: cb_generic_t TODO: Explanation
 *            port - The port passed in by the client.
 *
 * Returns: 0 - Successful execution
 *          1 - Failed execution.
 */
int cl_ui_gui_start(cb_generic_t *cbs, int port);

#endif
