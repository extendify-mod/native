#ifdef __linux__
#include "GDialogRunner.h"

#include "log/log.hpp"

#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>

typedef struct {
	GtkDialog* dialog;
	gint response_id;
	GMainLoop* loop;
	gboolean destroyed;
} RunInfo;

static void shutdown_loop(RunInfo* ri) {
	if (g_main_loop_is_running(ri->loop))
		g_main_loop_quit(ri->loop);
}

static void run_response_handler(GtkDialog* dialog, gint response_id,
								 gpointer data) {
	RunInfo* ri;

	ri = data;

	ri->response_id = response_id;

	shutdown_loop(ri);
}

static void run_unmap_handler(GtkDialog* dialog, gpointer data) {
	RunInfo* ri = data;

	shutdown_loop(ri);
}

static gint run_delete_handler(GtkDialog* dialog, GdkEventAny* event,
							   gpointer data) {
	RunInfo* ri = data;

	shutdown_loop(ri);

	return TRUE; /* Do not destroy */
}

static void run_destroy_handler(GtkDialog* dialog, gpointer data) {
	RunInfo* ri = data;

	/* shutdown_loop will be called by run_unmap_handler */

	ri->destroyed = TRUE;
}

gint gtk_dialog_run_with_context(GtkDialog* dialog, GMainContext* context) {
	RunInfo ri = {.dialog = NULL,
				  .response_id = GTK_RESPONSE_NONE,
				  .loop = NULL,
				  .destroyed = FALSE};
	gboolean was_modal;
	gulong response_handler;
	gulong unmap_handler;
	gulong destroy_handler;
	gulong delete_handler;

	E_ASSERT(context && "GMainContext must not be null");

	// NOLINTNEXTLINE(bugprone-reserved-identifier)
	g_return_val_if_fail(GTK_IS_DIALOG(dialog), -1);

	g_object_ref(dialog);

	was_modal = gtk_window_get_modal(GTK_WINDOW(dialog));

	if (!was_modal) {
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	}

	if (!gtk_widget_get_visible(GTK_WIDGET(dialog))) {
		gtk_widget_show(GTK_WIDGET(dialog));
	}

	response_handler = g_signal_connect(
		dialog, "response", G_CALLBACK(run_response_handler), &ri);

	unmap_handler =
		g_signal_connect(dialog, "unmap", G_CALLBACK(run_unmap_handler), &ri);

	delete_handler = g_signal_connect(
		dialog, "delete-event", G_CALLBACK(run_delete_handler), &ri);

	destroy_handler = g_signal_connect(
		dialog, "destroy", G_CALLBACK(run_destroy_handler), &ri);

	ri.loop = g_main_loop_new(context, FALSE);

	gdk_threads_leave();
	g_main_loop_run(ri.loop);
	gdk_threads_enter();

	g_main_loop_unref(ri.loop);

	if (!ri.destroyed) {
		if (!was_modal) {
			gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);
		}

		g_signal_handler_disconnect(dialog, response_handler);
		g_signal_handler_disconnect(dialog, unmap_handler);
		g_signal_handler_disconnect(dialog, delete_handler);
		g_signal_handler_disconnect(dialog, destroy_handler);
	}

	g_object_unref(dialog);

	return ri.response_id;
}
#endif
