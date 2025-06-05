#pragma once

#include "log/Logger.hpp"

#include <cef_callback.h>
#include <cef_v8.h>

namespace Extendify::api::settings {
	extern log::Logger logger;

	CefRefPtr<CefV8Value> makeApi();

	std::string readSettingsFile();

	void writeSettingsFile(const std::string& settings);

	CefRefPtr<CefV8Value> parseSettings(const std::string& settings);

	extern const std::string defaultSettingsJSON;
} // namespace Extendify::api::settings
