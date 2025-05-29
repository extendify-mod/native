#include "api.hpp"

#include "log/log.hpp"
#include "quickCss.hpp"
#include "settings.hpp"
#include "themes.hpp"
#include "util/iter.hpp"
#include "util/string.hpp"
#include "util/TaskCBHandler.hpp"
#include "util/util.hpp"

#include <cef_v8.h>
#include <include/cef_process_message.h>
#include <include/internal/cef_ptr.h>
#include <include/internal/cef_types.h>
#include <utility>

namespace Extendify::api {
	log::Logger logger({"Extendify", "api"});

	void inject(const CefRefPtr<CefV8Context>& context) {
		logger.info("Injecting ExtendifyNative API into V8 context");
		CefRefPtr<CefV8Value> global = context->GetGlobal();
		CefRefPtr<CefV8Value> extendify = CefV8Value::CreateObject(nullptr, nullptr);

		extendify->SetValue("settings", settings::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		extendify->SetValue("quickCss", quickCss::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		extendify->SetValue("themes", themes::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		global->SetValue("ExtendifyNative", extendify, V8_PROPERTY_ATTRIBUTE_NONE);
		global->SetValue(
			"EXTENDIFY_NATIVE_AVAILABLE", CefV8Value::CreateBool(true), V8_PROPERTY_ATTRIBUTE_NONE);
	}

	[[nodiscard]] V8Type getV8Type(const CefRefPtr<CefV8Value>& value) {
		E_ASSERT(value->IsValid() && "V8 value is not valid");

		if (value->IsUndefined()) {
			return V8Type::UNDEFINED;
		} else if (value->IsNull()) {
			return V8Type::NULL_TYPE;
		} else if (value->IsBool()) {
			return V8Type::BOOL;
		} else if (value->IsInt()) {
			return V8Type::INT;
		} else if (value->IsUInt()) {
			return V8Type::UINT;
		} else if (value->IsDouble()) {
			return V8Type::DOUBLE;
		} else if (value->IsDate()) {
			return V8Type::DATE;
		} else if (value->IsString()) {
			return V8Type::STRING;
		} else if (value->IsArray()) {
			return V8Type::ARRAY;
		} else if (value->IsArrayBuffer()) {
			return V8Type::ARRAY_BUFFER;
		} else if (value->IsFunction()) {
			return V8Type::FUNCTION;
		} else if (value->IsPromise()) {
			return V8Type::PROMISE;
		} else if (value->IsObject()) {
			return V8Type::OBJECT;
		}
		E_ASSERT(false && "Unknown V8 value type");
	};

	[[nodiscard]] std::string getTypeName(const CefRefPtr<CefV8Value>& value) {
		return getTypeName(getV8Type(value));
	};

	[[nodiscard]] constexpr std::string getTypeName(V8Type type) {
		switch (type) {
			case V8Type::UNDEFINED:
				return "undefined";
			case V8Type::NULL_TYPE:
				return "null";
			case V8Type::BOOL:
				return "bool";
			case V8Type::INT:
				return "int";
			case V8Type::UINT:
				return "uint";
			case V8Type::DOUBLE:
				return "double";
			case V8Type::DATE:
				return "date";
			case V8Type::STRING:
				return "string";
			case V8Type::OBJECT:
				return "object";
			case V8Type::ARRAY:
				return "array";
			case V8Type::ARRAY_BUFFER:
				return "array_buffer";
			case V8Type::FUNCTION:
				return "function";
			case V8Type::PROMISE:
				return "promise";
		}
		E_ASSERT(false && "Unknown v8 type");
	}

	[[nodiscard]] APIUsage::APIUsage(APIFunction func):
		func(std::move(func)) {
	}

	[[nodiscard]] constexpr std::string APIUsage::getUsage() const noexcept {
		if (func.name.empty()) {
			return "unknown usage";
		}
		std::ostringstream ret;
		if (!func.path.empty()) {
			ret << func.path << "#";
		}
		ret << func.name << "(";
		const auto args = typesToString(func.expectedArgs);
		ret << util::string::join(args, ", ") << ")";
		if (func.returnType) {
			ret << ": " << stringifyUnionType(*func.returnType);
		}
		return ret.str();
	}

	void APIUsage::validateOrThrow(const CefV8ValueList& arguments) const noexcept(false) {
		if (auto err = validateArgs(arguments)) {
			throw std::runtime_error(*err);
		}
	};

	[[nodiscard]] std::optional<std::string>
	APIUsage::validateArgs(const CefV8ValueList& arguments) const noexcept {
		if (arguments.size() < func.expectedArgs.size()
			|| (func.expectedArgs.size() != arguments.size() && !func.allowTrailingArgs)) {
			return std::format("expected {} {} arguments, got {}",
							   func.allowTrailingArgs ? "at least" : "exactly",
							   func.expectedArgs.size(),
							   arguments.size());
		}
		for (auto i = 0; i < arguments.size(); i++) {
			const auto& arg = arguments[i];
			if (getV8Type(arguments[i]) != func.expectedArgs[i]) {
				goto err;
			}
		}
		return {};
err:
		return std::format(
			"Invalid usage. Expected {}. Got {}", getUsage(), makeActualUsageString(arguments));
	}

	[[nodiscard]] std::string
	APIUsage::makeActualUsageString(const CefV8ValueList& arguments) const noexcept {
		std::string ret;
		if (!func.path.empty()) {
			ret += func.path + "#";
		}
		if (func.name.empty()) {
			ret += "<func_name>";
		} else {
			ret += func.name;
		}
		ret += "(";
		ret += util::string::join(util::iter::map(arguments,
												  [](const CefRefPtr<CefV8Value>& arg) {
													  return getTypeName(getV8Type(arg));
												  }),
								  ", ");
		ret += ")";
		if (func.returnType) {
			ret += ": " + stringifyUnionType(*func.returnType);
		}
		return ret;
	}

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<V8Type>& types) noexcept {
		return util::iter::map(types, [](const V8Type& type) -> std::string {
			return getTypeName(type);
			;
		});
	}

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<uint64_t>& types) noexcept {
		return util::iter::map(
			types, [](uint64_t type) -> std::string { return stringifyUnionType(type); });
	}

	[[nodiscard]] constexpr std::string APIUsage::stringifyUnionType(uint64_t type) noexcept {
		if (!type) {
			return {};
		}
		std::vector<V8Type> types;
		for (auto i = 1ULL; i < 1ULL << 63 && type; i <<= 1) {
			if (type & i) {
				type &= ~i;
				types.push_back(static_cast<V8Type>(i));
			}
		}
		return util::string::join(typesToString(types), " | ");
	}

	CefRefPtr<CBHandler> CBHandler::Create(Callback h) {
		CefRefPtr<CBHandler> ret = new CBHandler;
		ret->setCallback(std::move(h));
		return ret;
	}

	bool CBHandler::Execute(CB_HANDLER_ARGS) {
		return handler(name, object, arguments, retval, exception);
	}

	void CBHandler::setCallback(Callback h) {
		handler = std::move(h);
	}

	ScopedV8Context::ScopedV8Context(CefRefPtr<CefV8Context> context):
		id(nextId++),
		context(std::move(context)) {
		if (!this->context->IsValid()) {
			E_ASSERT(false && "trying to use ScopedV8Context with an invalid context");
		}
		if (CefV8Context::GetEnteredContext()->IsSame(this->context)) {
			shouldExit = false;
		} else {
			context->Enter();
		}
		logger.trace("Entering ScopedV8Context {}, shouldExit: {}", id, shouldExit);
	}

	ScopedV8Context::~ScopedV8Context() {
		if (shouldExit) {
			context->Exit();
		}
		logger.trace("Exiting ScopedV8Context {}, shouldExit: {}", id, shouldExit);
	}

	log::Logger ScopedV8Context::logger {{"Extendify", "api", "ScopedV8Context"}};
	int ScopedV8Context::nextId = 1;

	// NOLINTNEXTLINE(performance-unnecessary-value-param)
	[[nodiscard]] CefRefPtr<CefV8Value> FilePicker::launch(CefRefPtr<CefV8Context> context,
														   Callback callback) const {
		ScopedV8Context ctx(context);
		E_ASSERT(util::isOnProcess(PID_BROWSER)
				 && "FilePicker can only be used in the browser process");

		auto promise = CefV8Value::CreatePromise();
		CefTaskRunner::GetForThread(CefThreadId::TID_UI)
			->PostTask(util::TaskCBHandler::Create([browser = context->GetBrowser(),
													mode = mode,
													title = title,
													defaultFilePath = defaultFilePath,
													acceptFilters = acceptFilters,
													context,
													promise,
													callback]() {
				auto host = browser->GetHost();
				E_ASSERT(host != nullptr && "Browser host is null in FilePicker::launch");
				host->RunFileDialog(
					mode,
					title,
					defaultFilePath.string(),
					util::iter::map(acceptFilters, util::into<CefString> {}),
					FilePickerCallback::Create(context, promise, std::move(callback)));
			}));
		return promise;
	}

	[[nodiscard]] CefRefPtr<CefV8Value> FilePicker::pickOne(CefRefPtr<CefV8Context> context,
															Callback callback) {
		static FilePicker picker {
			.mode = FileDialogMode::FILE_DIALOG_OPEN,
		};
		return picker.launch(context, std::move(callback));
	}

	[[nodiscard]] CefRefPtr<CefV8Value> FilePicker::pickMultiple(CefRefPtr<CefV8Context> context,
																 Callback callback) {
		static FilePicker picker {
			.mode = FileDialogMode::FILE_DIALOG_OPEN_MULTIPLE,
		};
		return picker.launch(context, std::move(callback));
	}

	[[nodiscard]] CefRefPtr<CefV8Value> FilePicker::pickFolder(CefRefPtr<CefV8Context> context,
															   Callback callback) {
		static FilePicker picker {
			.mode = FileDialogMode::FILE_DIALOG_OPEN_FOLDER,
		};
		return picker.launch(context, std::move(callback));
	}

	[[nodiscard]] CefRefPtr<CefV8Value> FilePicker::pickSaveFile(CefRefPtr<CefV8Context> context,
																 Callback callback) {
		static FilePicker picker {
			.mode = FileDialogMode::FILE_DIALOG_SAVE,
		};
		return picker.launch(context, std::move(callback));
	}

	void
	FilePicker::FilePickerCallback::OnFileDialogDismissed(const std::vector<CefString>& filePaths) {
		E_ASSERT(callback && "FilePickerCallback called without a callback set");
		CefTaskRunner::GetForThread(CefThreadId::TID_RENDERER)
			->PostTask(util::TaskCBHandler::Create([context = context,
													promise = promise,
													callback = callback,
													filePaths = filePaths]() {
				E_ASSERT(context->IsValid() && "invalid context in FilePickerCallback");
				ScopedV8Context ctx(context);
				context->GetTaskRunner()->PostTask(util::TaskCBHandler::Create([=]() {
					E_ASSERT(context->IsValid() && "invalid context in FilePickerCallback");
					ScopedV8Context ctx(context);
					E_ASSERT(promise->IsValid() && "FilePickerCallback promise is not valid");
					std::expected<scoped_refptr<CefV8Value>, std::basic_string<char>> ret;
					try {
						ret = callback(
							util::iter::map(filePaths, util::into<std::filesystem::path> {}));
					} catch (const std::exception& e) {
						std::string msg = std::format("Error in FilePickerCallback: {}", e.what());
						logger.error(msg);
						ret = std::unexpected(msg);
					}
					if (ret.has_value()) {
						promise->ResolvePromise(std::move(ret.value()));
					} else {
						promise->RejectPromise(ret.error());
					}
				}));
			}));
	};

	[[nodiscard]] CefRefPtr<FilePicker::FilePickerCallback>
	FilePicker::FilePickerCallback::Create(CefRefPtr<CefV8Context> context,
										   CefRefPtr<CefV8Value> promise, Callback callback) {
		auto ret = new FilePickerCallback;
		ret->callback = std::move(callback);
		ret->promise = std::move(promise);
		ret->context = std::move(context);
		return ret;
	}
} // namespace Extendify::api
