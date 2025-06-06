#ifdef __linux__
#include "FileChooserDialogCtx.hpp"

#include "GDialogRunner.h"

namespace Extendify::fs {
	FileChooserDialogWithContext::FileChooserDialogWithContext(
		const Glib::ustring& title, Gtk::FileChooserAction action,
		Gtk::DialogFlags flags):
		Gtk::FileChooserDialog(title, action, flags) { };
	FileChooserDialogWithContext::FileChooserDialogWithContext(
		const Glib::ustring& title, Gtk::FileChooserAction action):
		Gtk::FileChooserDialog(title, action) {

		};

	void FileChooserDialogWithContext::setContext(
		Glib::RefPtr<Glib::MainContext> ctx) {
		context = std::move(ctx);
	}

	Glib::RefPtr<Glib::MainContext>
	FileChooserDialogWithContext::getContext() const {
		return context;
	}

	int FileChooserDialogWithContext::run() {
		return gtk_dialog_run_with_context(Gtk::Dialog::gobj(),
										   context ? context->gobj() : nullptr);
	}
} // namespace Extendify::fs
#endif
