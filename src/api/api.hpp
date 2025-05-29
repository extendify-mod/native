#pragma once
#include "log/Logger.hpp"

#include <cef_thread.h>
#include <cef_v8.h>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>

#define CB_HANDLER_ARGS                                                \
	const CefString &name, CefRefPtr<CefV8Value> object,               \
		const CefV8ValueList &arguments, CefRefPtr<CefV8Value>&retval, \
		CefString &exception

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

		void validateOrThrow(const CefV8ValueList& arguments) const
			noexcept(false);

		[[nodiscard]] std::optional<std::string>
		validateArgs(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] std::string
		makeActualUsageString(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] static std::string
		makeUsageString(const APIFunction& func) noexcept;

		/**
		 * @brief converts a vector of V8Type to a vector of strings for each
		 * type
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

		[[nodiscard]] constexpr static std::string
		stringifyUnionType(uint64_t type) noexcept;

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

	/**
	 * YOU ARE RESPONSIBLE FOR MAKING SURE THAT THIS LIVES LONGER THAN THE
	 * PROMISE
	 */
	class FilePicker: public CefBaseRefCounted {
	  public:
		enum class DialogType {
			OPEN,
			OPEN_FOLDER,
			SAVE
		};
		typedef std::function<std::expected<CefRefPtr<CefV8Value>, std::string>(
			std::vector<std::filesystem::path>)>
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
		launch(CefRefPtr<CefV8Context> context, Callback callback);
		// utility functions that just use the default values

		[[nodiscard]] static CefRefPtr<FilePicker> pickOne();
		[[nodiscard]] static CefRefPtr<FilePicker> pickFolder();
		[[nodiscard]] static CefRefPtr<FilePicker> pickSaveFile();

	  private:
		// ctor
		[[nodiscard]] explicit FilePicker(FilePickerData data);
		// static
		static log::Logger logger;
		static int nextId;
		// config
		DialogType mode;
		// impl
		int id = nextId++;
		Callback callback;
		CefRefPtr<CefV8Value> promise;
		CefRefPtr<CefV8Context> context;
		CefRefPtr<CefThread> pickerThread;
		void runFilePicker();
		std::expected<std::optional<std::filesystem::path>, std::string>
		showDialog();
		IMPLEMENT_REFCOUNTING(FilePicker);
	};
} // namespace Extendify::api
