#ifdef __linux__
#pragma once

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

gint gtk_dialog_run_with_context(GtkDialog* dialog, GMainContext* context);

#ifdef __cplusplus
}
#endif
#endif
