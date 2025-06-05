#include "settings.hpp"

#include "fs/fs.hpp"
#include "log/Logger.hpp"
#include "path/path.hpp"
#include "util/APIFunction.hpp"
#include "util/APIUsage.hpp"
#include "util/CBHandler.hpp"
#include "util/json.hpp"

#include <cef_base.h>
#include <cef_v8.h>

namespace Extendify::api::settings {
	using util::APIFunction;
	using util::APIUsage;
	using util::CBHandler;
	using util::V8Type;

	namespace usage {
		const APIUsage get {APIFunction {
			.name = "get",
			.description = "Get the settings object",
			.path = "settings",
			.expectedArgs = {},
			.returnType = V8Type::OBJECT,
		}};
		const APIUsage set {APIFunction {
			.name = "set",
			.description = "Set the settings object",
			.path = "settings",
			.expectedArgs = {V8Type::OBJECT},
			.returnType = V8Type::UNDEFINED,
		}};
		const APIUsage getSettingsDir {APIFunction {
			.name = "getSettingsDir",
			.description = "Get the settings directory",
			.path = "settings",
			.expectedArgs = {},
			.returnType = V8Type::STRING,
		}};
	} // namespace usage

	log::Logger logger({"Extendify", "api", "settings"});

	static auto getHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
		   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
		   CefString& exception) {
			try {
				usage::get.validateOrThrow(arguments);
				const auto settingsString = readSettingsFile();
				auto settingsObject = parseSettings(settingsString);
				retval = settingsObject;
			} catch (const std::exception& e) {
				settings::logger.error("Error in getSettings: {}", e.what());
				exception = e.what();
			}
			return true;
		});

	// NOLINTNEXTLINE(performance-unnecessary-value-param)
	static auto setHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
		   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& /*retval*/,
		   CefString& exception) {
			try {
				usage::set.validateOrThrow(arguments);
				const auto& obj = arguments[0];
				const auto settingsString = util::json::stringify(obj);
				writeSettingsFile(settingsString);

			} catch (const std::exception& e) {
				settings::logger.error("Error in setSettings: {}", e.what());
				exception = e.what();
			}
			return true;
		});

	static auto getSettingsDirHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
		   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
		   CefString& exception) {
			try {
				usage::getSettingsDir.validateOrThrow(arguments);
				const auto settingsPath =
					path::getConfigFilePath().parent_path();
				const auto ret =
					CefV8Value::CreateString(settingsPath.string());
				retval = ret;
			} catch (const std::exception& e) {
				settings::logger.error("Error in getSettingsDir: {}", e.what());
				exception = e.what();
			}
			return true;
		});

	CefRefPtr<CefV8Value> makeApi() {
		CefRefPtr<CefV8Value> api = CefV8Value::CreateObject(nullptr, nullptr);

		auto getFunc = CefV8Value::CreateFunction("get", getHandler);
		api->SetValue("get", getFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		auto setFunc = CefV8Value::CreateFunction("set", setHandler);
		api->SetValue("set", setFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		auto getSettingsDirFunc =
			CefV8Value::CreateFunction("getSettingsDir", getSettingsDirHandler);
		api->SetValue(
			"getSettingsDir", getSettingsDirFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		return api;
	}

	std::string readSettingsFile() {
		auto settingsPath = path::getConfigFilePath(true);
		auto contents = fs::readFile(settingsPath);
		return contents;
	};

	void writeSettingsFile(const std::string& settings) {
		fs::writeFile(path::getConfigFilePath(true), settings);
	}

	CefRefPtr<CefV8Value> parseSettings(const std::string& settings) {
		return util::json::parse(settings);
	};

	const std::string defaultSettingsJSON = R"({
        "plugins": {},
        "theme": {
            "files": [],
            "colors": []
        }
    })";
} // namespace Extendify::api::settings
