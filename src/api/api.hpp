#pragma once
#include "log/log.hpp"
#include "log/Logger.hpp"
#include "util/iter.hpp"

#include <cef_base.h>
#include <cef_callback.h>
#include <cef_v8.h>
#include <cstdint>
#include <filesystem>
#include <include/cef_base.h>
#include <include/cef_browser.h>
#include <include/internal/cef_ptr.h>
#include <optional>
#include <string>

#define CB_HANDLER_ARGS                                                                   \
	const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, \
		CefRefPtr<CefV8Value>&retval, CefString &exception

namespace Extendify::api {
	extern log::Logger logger;
	void inject(const CefRefPtr<CefV8Context>& context);

	enum V8Type {
		UNDEFINED = 1,
		NULL_TYPE = 2 << 0,
		BOOL = 2 << 1,
		INT = 2 << 2,
		UINT = 2 << 3,
		DOUBLE = 2 << 4,
		DATE = 2 << 5,
		STRING = 2 << 6,
		OBJECT = 2 << 7,
		ARRAY = 2 << 8,
		ARRAY_BUFFER = 2 << 9,
		FUNCTION = 2 << 10,
		PROMISE = 2 << 11,
	};

	/**
	 * @brief gets the type for a v8 value
	 *
	 * @param value a v8 value
	 * @invariant @param value is valid
	 * @return V8Type
	 */
	[[nodiscard]] V8Type getV8Type(const CefRefPtr<CefV8Value>& value);

	/**
	 * @brief Get the type of value
	 *
	 * @param value
	 * @invariant @param value is valid
	 * @return const std::string&
	 */
	[[nodiscard]] std::string getTypeName(const CefRefPtr<CefV8Value>& value);
	[[nodiscard]] constexpr std::string getTypeName(V8Type type);

	struct APIFunction {
		std::string name;
		std::string description;
		std::string path;
		bool allowTrailingArgs = false;
		/**
		 * @brief array of parameter types, bitwise OR'd together
		 * @see V8Type
		 */
		const std::vector<uint64_t> expectedArgs;
		/**
		 * @brief  return type, bitwise OR'd together
		 * @see V8Type
		 */
		std::optional<uint64_t> returnType;
	};

	class APIUsage {
	  public:
		// NOLINTNEXTLINE(google-explicit-constructor)
		[[nodiscard]] APIUsage(APIFunction func);

		[[nodiscard]] constexpr std::string getUsage() const noexcept;

		void validateOrThrow(const CefV8ValueList& arguments) const noexcept(false);

		[[nodiscard]] std::optional<std::string>
		validateArgs(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] std::string
		makeActualUsageString(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] static std::string makeUsageString(const APIFunction& func) noexcept;

		/**
		 * @brief converts a vector of V8Type to a vector of strings for each type
		 *
		 * does not accept union types
		 *
		 * @param types the types to convert
		 * @return std::vector<std::string>
		 */
		[[nodiscard]] constexpr static std::vector<std::string>
		typesToString(const std::vector<V8Type>& types) noexcept;

		[[nodiscard]] constexpr static std::vector<std::string>
		typesToString(const std::vector<uint64_t>& types) noexcept;

		[[nodiscard]] constexpr static std::string stringifyUnionType(uint64_t type) noexcept;

	  private:
		const APIFunction func;
	};

	template<typename... Args>
	class CallbackManager { };

	class CBHandler final: public CefV8Handler {
	  public:
		typedef std::function<bool(CB_HANDLER_ARGS)> Callback;

		static CefRefPtr<CBHandler> Create(Callback h);

		bool Execute(CB_HANDLER_ARGS) override;

	  private:
		void setCallback(Callback h);
		Callback handler;
		IMPLEMENT_REFCOUNTING(CBHandler);
	};

	class ScopedV8Context {
	  public:
		explicit ScopedV8Context(CefRefPtr<CefV8Context> context);

		~ScopedV8Context();

		ScopedV8Context() = delete;
		ScopedV8Context(const ScopedV8Context& other) = delete;
		ScopedV8Context(ScopedV8Context&& other) = delete;

	  private:
		static log::Logger logger;
		static int nextId;
		int id;
		bool shouldExit = true;
		CefRefPtr<CefV8Context> context;
	};

	class FilePicker {
	  public:
		typedef cef_file_dialog_mode_t FileDialogMode;
		typedef std::function<std::optional<CefRefPtr<CefV8Value>>(
			std::vector<std::filesystem::path>)>
			Callback;
		FileDialogMode mode = FileDialogMode::FILE_DIALOG_OPEN;
		std::string title;
		std::filesystem::path defaultFilePath;
		// cursed comment becuase of /* in block comments

		/**
		 * used to restrict the selectable file types and may any combination of (a) valid */
		/// lower-cased MIME types (e.g. "text/*" or "image/*"), (b) individual file extensions
		/** (e.g. ".txt" or ".png"), or (c) combined description and file extension delimited using
		 * "|" and
		 * ";" (e.g. "Image Types|.png;.gif;.jpg").
		 */
		std::vector<std::string> acceptFilters;
		/**
		 * @brief runs the file picker dialog
		 *
		 * @param context the context that the promise will be created in
		 * @param callback will be called once the user has selected files, will be called with the
		 * path of each file/folder
		 * @invariant context is valid for this thread
		 * @return CefRefPtr<CefV8Value>
		 */
		[[nodiscard]] CefRefPtr<CefV8Value> launch(CefRefPtr<CefV8Context> context,
												   Callback callback) const;
		// utility functions that just use the default values

		[[nodiscard]] static CefRefPtr<CefV8Value> pickOne(CefRefPtr<CefV8Context> context,
														   Callback callback);
		[[nodiscard]] static CefRefPtr<CefV8Value> pickMultiple(CefRefPtr<CefV8Context> context,
																Callback callback);
		[[nodiscard]] static CefRefPtr<CefV8Value> pickFolder(CefRefPtr<CefV8Context> context,
															  Callback callback);
		[[nodiscard]] static CefRefPtr<CefV8Value> pickSaveFile(CefRefPtr<CefV8Context> context,
																Callback callback);

	  private:
		class FilePickerCallback final: public CefRunFileDialogCallback {
		  public:
			void OnFileDialogDismissed(const std::vector<CefString>& filePaths) override;
			[[nodiscard]] static CefRefPtr<FilePickerCallback> Create(CefRefPtr<CefV8Context> context,
																	  CefRefPtr<CefV8Value> promise,
																	  Callback callback);

		  private:
			CefRefPtr<CefV8Value> promise;
			CefRefPtr<CefV8Context> context;
			Callback callback = nullptr;
			IMPLEMENT_REFCOUNTING(FilePickerCallback);
		};
	};
} // namespace Extendify::api
