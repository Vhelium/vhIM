#include "client_ui_gui_window.h"

G_DEFINE_TYPE(ClientGuiWindow, cl_ui_gui_window, GTK_TYPE_APPLICATION_WINDOW);

static void cl_ui_gui_window_init(ClientGuiWindow *win)
{
}

static void cl_ui_gui_window_class_init(ClientGuiWindowClass *class)
{
}

ClientGuiWindow * cl_ui_gui_window_new(ClientGui *app)
{
    return g_object_new(CLIENT_UI_GUI_WINDOW_TYPE, "application", app, NULL);
}

void cl_ui_gui_window_open(ClientGuiWindow *win, GFile *file)
{
}
