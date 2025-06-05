#pragma once

#include "log/Logger.hpp"

#include <cef_callback.h>
#include <cef_v8.h>
#include <internal/cef_ptr.h>
#include <string>

/**
 * @brief QuickCSS api
 *
 */
namespace Extendify::api::quickCss {
	extern log::Logger logger;

	CefRefPtr<CefV8Value> makeApi();

	std::string readQuickCssFile();

	void writeQuickCssFile(const std::string& contents);

	void openQuickCssFile();

	/**
	 * @brief Dispatches a quickcss update to all registered listeners
	 * this may be called from any thread
	 *
	 * the content dispatched is the result of @see readQuickCssFile
	 */
	void dispatchQuickCssUpdate();
	/**
	 * @brief Dispatches a quickcss update to all registered listeners
	 * this may be called from any thread
	 *
	 */
	void dispatchQuickCssUpdate(const std::string& content);
} // namespace Extendify::api::quickCss
