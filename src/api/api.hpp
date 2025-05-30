#pragma once
#include "log/Logger.hpp"

#include <base/cef_ref_counted.h>
#include <cef_thread.h>
#include <cef_v8.h>

namespace Extendify::api {
	extern log::Logger logger;
	void inject(const CefRefPtr<CefV8Context>& context);
} // namespace Extendify::api
