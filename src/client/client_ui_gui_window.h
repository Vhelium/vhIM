#ifndef __CLIENT_UI_GUI_WINDOW_H_
#define __CLIENT_UI_GUI_WINDOW_H_

#include "client_ui_gui.h"

#define CLIENT_UI_GUI_WINDOW_TYPE (cl_ui_gui_window_get_type())
#define CLIENT_UI_GUI_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), CLIENT_UI_GUI_WINDOW_TYPE, ClientGuiWindow))

typedef struct cl_ui_gui_window {
    GtkApplicationWindow parent;
} ClientGuiWindow;

typedef struct cl_ui_gui_window_class {
    GtkApplicationWindowClass parent_class;
} ClientGuiWindowClass;

GType cl_ui_gui_window_get_type();

ClientGuiWindow * cl_ui_gui_window_new(ClientGui *app);

void cl_ui_gui_window_open(ClientGuiWindow *win, GFile *file);

#endif /* __CLIENT_UI_GUI_WINDOW_H_ */
