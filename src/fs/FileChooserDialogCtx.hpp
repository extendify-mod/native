#ifdef __linux__
#pragma once

#include <gtkmm.h>

namespace Extendify::fs {
	class FileChooserDialogWithContext: public Gtk::FileChooserDialog {
	  public:
		explicit FileChooserDialogWithContext(
			const Glib::ustring& title,
			Gtk::FileChooserAction action = Gtk::FILE_CHOOSER_ACTION_OPEN);

		explicit FileChooserDialogWithContext(const Glib::ustring& title,
											  Gtk::FileChooserAction action,
											  Gtk::DialogFlags flags);

		~FileChooserDialogWithContext() noexcept override = default;

		void setContext(Glib::RefPtr<Glib::MainContext> ctx);
		Glib::RefPtr<Glib::MainContext> getContext() const;

		int run();

	  private:
		Glib::RefPtr<Glib::MainContext> context {};
	};
} // namespace Extendify::fs
#endif