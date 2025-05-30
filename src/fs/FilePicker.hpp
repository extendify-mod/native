#pragma once

#include <cef_base.h>
#include <cef_thread.h>
#include <cef_v8.h>
#include <expected>
#include <filesystem>
#include <functional>
#include <variant>

namespace Extendify::fs {
	/**
	 * YOU ARE RESPONSIBLE FOR MAKING SURE THAT THIS LIVES LONGER THAN THE
	 * PROMISE
	 */
	class FilePicker: public CefBaseRefCounted {
	  public:
		enum class DialogType {
			OPEN,
			OPEN_MANY,
			OPEN_FOLDER,
			SAVE
		};
		// the first arg is always a ref to the file picker that this was
		// launched from, the second the result if the array is empty,
		// somethoing went wrong or the user cancelled the dialog
		// If an error occurs, the third argument will contain an error message
		typedef std::function<std::expected<CefRefPtr<CefV8Value>, std::string>(
			CefRefPtr<FilePicker>, std::vector<std::filesystem::path>, std::optional<std::string>)>
			PromiseCallback;
		typedef std::function<void(CefRefPtr<FilePicker>,
								   std::vector<std::filesystem::path>, std::optional<std::string>)>
			Callback;

		struct FilePickerData {
			DialogType mode = DialogType::OPEN;
			std::string title;
			std::filesystem::path defaultFilePath;
			std::vector<std::string> acceptFilters;
		};

		FilePicker() = delete;

		[[nodiscard]] static CefRefPtr<FilePicker> Create(FilePickerData data);

		/**
		 * @brief runs the file picker dialog
		 *
		 * @param context the context that the promise will be created in
		 * @param callback will be called once the user has selected files, will
		 * be called with the path of each file/folder **WILL BE CALLED IN THE
		 * SAME CONTEXT PROVIDED**
		 * @invariant context is valid for this thread
		 * @return CefRefPtr<CefV8Value>
		 */
		[[nodiscard]] CefRefPtr<CefV8Value>
		promise(CefRefPtr<CefV8Context> context, PromiseCallback callback);

		void launch(Callback callback);
		// utility functions that just use the default values

		[[nodiscard]] static CefRefPtr<FilePicker> pickOne();
		[[nodiscard]] static CefRefPtr<FilePicker> pickMany();
		[[nodiscard]] static CefRefPtr<FilePicker> pickFolder();
		[[nodiscard]] static CefRefPtr<FilePicker> pickSaveFile();

	  private:
		// ctor
		[[nodiscard]] explicit FilePicker(FilePickerData data);
		// static
		static int nextId;
		// config
		DialogType mode;
		// impl
		int id = nextId++;
		struct V8Data {
			CefRefPtr<CefV8Context> context;
			CefRefPtr<CefV8Value> promise;
			PromiseCallback callback;
		};
		struct RawData {
			Callback callback;
		};
		std::variant<V8Data, RawData> data;
		CefRefPtr<CefThread> pickerThread;

		CefRefPtr<FilePicker> self;

		void runFilePicker();
		std::expected<std::vector<std::filesystem::path>, std::string>
		showDialog();
		IMPLEMENT_REFCOUNTING(FilePicker);
	};
} // namespace Extendify::fs
