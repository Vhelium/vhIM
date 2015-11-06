#ifndef CLIENT_UI_GUI_H
#define CLIENT_UI_GUI_H

#include "client_callback.h"

#include <gtk/gtk.h>

/* ================== Structs ================== */

/*
 * Struct: ClientGuiApp
 * --------------------
 * Description:
 * The basic application type.
 */
typedef struct ClientGuiApp {
    GtkWidget *main_window, *menu_bar, *status_bar;
    guint status_bar_context_id;
} ClientGuiApp;

/* ================== Functions ================ */

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
