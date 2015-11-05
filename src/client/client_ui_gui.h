#ifndef CLIENT_UI_GUI_H
#define CLIENT_UI_GUI_H

#include "client_callback.h"

#include <gtk/gtk.h>

#define CLIENT_UI_GUI_TYPE (cl_ui_gui_get_type())
#define CLIENT_UI_GUI(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), CLIENT_UI_GUI_TYPE, ClientGui))

typedef struct cl_ui_gui {
    GtkApplication parent;
} ClientGui;

typedef struct cl_ui_gui_class {
    GtkApplicationClass parent_class;
} ClientGuiClass;

GType cl_ui_gui_get_type();

struct cl_ui_gui * cl_ui_gui_new();

int cl_ui_gui_start(cb_generic_t *cbs, int port);

#endif
