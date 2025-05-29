#include "quickCss.hpp"

#include "api/api.hpp"
#include "fs/fs.hpp"
#include "fs/Watcher.hpp"
#include "log/Logger.hpp"
#include "path/path.hpp"
#include "util/TaskCBHandler.hpp"

#include <cef_base.h>
#include <cef_v8.h>
#include <exception>
#include <memory>

using namespace Extendify;
using namespace api;

static CefV8ValueList quickCssChangeListeners {};
static CefRefPtr<CefV8Context> quickCssContext = nullptr;

namespace Extendify::api::quickCss {
	log::Logger logger({"Extendify", "api", "quickCss"});

	namespace usage {
		APIUsage get {APIFunction {
			.name = "get",
			.description = "Get the users quick css",
			.path = "quickCss",
			.expectedArgs = {},
			.returnType = V8Type::STRING,
		}};
		APIUsage set {APIFunction {
			.name = "set",
			.description = "Set the users quick css",
			.path = "quickCss",
			.expectedArgs = {V8Type::STRING},
			.returnType = V8Type::UNDEFINED,
		}};
		APIUsage addChangeListener {APIFunction {
			.name = "addChangeListener",
			.description = "Add a listener for quick css changes",
			.path = "quickCss",
			.expectedArgs = {V8Type::FUNCTION},
			.returnType = V8Type::UNDEFINED,
		}};
		APIUsage openFile {APIFunction {
			.name = "openFile",
			.description = "Open the quick css file in the default editor",
			.path = "quickCss",
			.expectedArgs = {},
			.returnType = V8Type::UNDEFINED,
		}};
	} // namespace usage

	// NOLINTNEXTLINE(performance-unnecessary-value-param)
	static auto getHandler = CBHandler::Create([](CB_HANDLER_ARGS) {
		try {
			usage::get.validateOrThrow(arguments);
			auto css = readQuickCssFile();
			retval = CefV8Value::CreateString(css);
		} catch (std::exception& e) {
			const auto msg = std::format("Error reading quick CSS file: {}", e.what());
			logger.error(msg);
			exception = msg;
		}
		return true;
	});

	static auto setHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments,
		   CefRefPtr<CefV8Value>& retval, CefString& exception) {
			try {
				usage::set.validateOrThrow(arguments);
				const auto content = arguments[0]->GetStringValue();
				writeQuickCssFile(content);
				// we dont need to dispatch an update here, as the file watcher will
			} catch (std::exception& e) {
				const auto msg = std::format("Error writing quick CSS file: {}", e.what());
				logger.error(msg);
				exception = msg;
			}
			return true;
		});

	static auto addChangeListenerHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments,
		   CefRefPtr<CefV8Value>& retval, CefString& exception) {
			try {
				usage::addChangeListener.validateOrThrow(arguments);
				const auto currentContext = CefV8Context::GetCurrentContext();
				if (!quickCssContext) {
					quickCssContext = currentContext;
				} else if (!quickCssContext->IsSame(currentContext)) {
					logger.error("Saved quick css context is not the same as the entered one while "
								 "adding a listener, invalidating all previous listeners and using "
								 "the current context");
					quickCssChangeListeners.clear();
					quickCssContext = currentContext;
				}
				quickCssChangeListeners.push_back(arguments[0]);
				logger.debug("Added quick css change listener, total listeners: {}",
							 quickCssChangeListeners.size());
			} catch (std::exception& e) {
				const auto msg = std::format("Error adding change listener: {}", e.what());
				logger.error(msg);
				exception = msg;
			}
			return true;
		});

	static auto openFileHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments,
		   CefRefPtr<CefV8Value>& retval, CefString& exception) {
			try {
				usage::openFile.validateOrThrow(arguments);
				openQuickCssFile();
			} catch (std::exception& e) {
				const auto msg = std::format("Error opening file: {}", e.what());
				logger.error(msg);
				exception = msg;
			}
			return true;
			;
		});

	CefRefPtr<CefV8Value> makeApi() {
		CefRefPtr<CefV8Value> api = CefV8Value::CreateObject(nullptr, nullptr);

		CefRefPtr<CefV8Value> getFunc = CefV8Value::CreateFunction("get", getHandler);
		api->SetValue("get", getFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> setFunc = CefV8Value::CreateFunction("set", setHandler);
		api->SetValue("set", setFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> addChangeListenerFunc =
			CefV8Value::CreateFunction("addChangeListener", addChangeListenerHandler);
		api->SetValue("addChangeListener", addChangeListenerFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> openFileFunc =
			CefV8Value::CreateFunction("openFile", openFileHandler);
		api->SetValue("openFile", openFileFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> openEditorFunc =
			CefV8Value::CreateFunction("openEditor", openFileHandler);
		api->SetValue("openEditor", openEditorFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		static std::optional<int> watcherId = {};
		if (!watcherId) {
			watcherId = fs::Watcher::get()->addFile(
				path::getQuickCssFile(), [](std::unique_ptr<fs::Watcher::Event> event) {
					logger.debug("change in quick css file; dispatching update");
					dispatchQuickCssUpdate();
				});
		}

		return api;
	}

	std::string readQuickCssFile() {
		return fs::readFile(path::getQuickCssFile(true));
	}

	void writeQuickCssFile(const std::string& contents) {
		fs::writeFile(path::getQuickCssFile(true), contents);
	}

	void openQuickCssFile() {
		fs::openPath(path::getQuickCssFile(true));
	};

	void dispatchQuickCssUpdate() {
		dispatchQuickCssUpdate(readQuickCssFile());
	};

	void dispatchQuickCssUpdate(const std::string& contents) {
		CefTaskRunner::GetForThread(CefThreadId::TID_RENDERER)
			->PostTask(util::TaskCBHandler::Create([contents]() {
				if (!quickCssContext) {
					logger.warn(
						"attempting to dispatch a quick css update before any listeners are setup");
					return;
				}
				if (!quickCssContext->IsValid()) {
					logger.error("quick css context is not valid");
					return;
				}
				quickCssContext->GetTaskRunner()->PostTask(
					util::TaskCBHandler::Create([contents]() -> void {
						for (const auto& cb : quickCssChangeListeners) {
							if (!cb->IsFunction()) {
								logger.warn(
									"cb in quick css change listeners that is not a function, this "
									"should never happen");
								continue;
							}
							cb->ExecuteFunctionWithContext(
								quickCssContext, nullptr, {CefV8Value::CreateString(contents)});
						}
					}));
			}));
	}

} // namespace Extendify::api::quickCss
