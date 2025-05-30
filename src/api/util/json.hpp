#pragma once

#include <cef_callback.h>
#include <cef_v8.h>

/**
 * @brief JSON parsing utilities
 *
 * NOTE: Must only be used within a v8 context as they rely on window.json
 */
namespace Extendify::api::util::json {
	CefRefPtr<CefV8Value> parse(const CefString& json);
	/**
	 * @param json must be a string
	 */
	CefRefPtr<CefV8Value> parse(const CefRefPtr<CefV8Value>& json);

	std::string stringify(const CefRefPtr<CefV8Value>& obj);
} // namespace Extendify::util::json
