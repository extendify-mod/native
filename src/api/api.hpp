#pragma once
#include "log/Logger.hpp"

#include <cef_base.h>
#include <cef_callback.h>
#include <cef_v8.h>

namespace Extendify::api {
	extern log::Logger logger;
	void inject(const CefRefPtr<CefV8Context>& context);
} // namespace Extendify::api
