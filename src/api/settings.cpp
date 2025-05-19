#include "settings.hpp"

#include "api.hpp"
#include "fs/fs.hpp"
#include "log/Logger.hpp"
#include "path/path.hpp"
#include "util/json.hpp"

#include <cef_base.h>
#include <cef_v8.h>

using namespace Extendify;
using namespace api;
using namespace settings;

namespace Extendify::api::settings {
	log::Logger logger({"Extendify", "api", "settings"});

	// NOLINTNEXTLINE(performance-unnecessary-value-param)
	static auto getHandler = CBHandler::Create([](CB_HANDLER_ARGS) {
		try {
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
	static auto setHandler = CBHandler::Create([](CB_HANDLER_ARGS) {
		try {
			const auto numArgs = arguments.size();
			if (numArgs != 1) {
				settings::logger.error("settings.set() requires 1 argument, got {}", numArgs);
				exception = "settings.set() requires 1 argument";
				return true;
			}
			const auto& obj = arguments[0];
			if (!obj->IsObject()) {
				settings::logger.error("settings.set() requires an object argument");
				exception = "settings.set() requires an object argument";
				return true;
			}
			const auto settingsString = util::json::stringify(obj);
			writeSettingsFile(settingsString);

		} catch (const std::exception& e) {
			settings::logger.error("Error in setSettings: {}", e.what());
			exception = e.what();
		}
		return true;
	});

	// NOLINTNEXTLINE(performance-unnecessary-value-param)
	static auto getSettingsDirHandler = CBHandler::Create([](CB_HANDLER_ARGS) {
		try {
			const auto settingsPath = path::getConfigFilePath().parent_path();
			const auto ret = CefV8Value::CreateString(settingsPath.string());
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

		auto getSettingsDirFunc = CefV8Value::CreateFunction("getSettingsDir", getSettingsDirHandler);
		api->SetValue("getSettingsDir", getSettingsDirFunc, V8_PROPERTY_ATTRIBUTE_NONE);

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
} // namespace api::settings
