#pragma once
#include "log/Logger.hpp"

#include <cef_v8.h>
#include <internal/cef_ptr.h>

namespace Extendify::api {
	extern log::Logger logger;
	void inject(const CefRefPtr<CefV8Context>& context);
} // namespace Extendify::api
