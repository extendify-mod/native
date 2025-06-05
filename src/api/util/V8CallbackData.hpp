#pragma once
#include <cef_v8.h>
#include <internal/cef_ptr.h>

namespace Extendify::api::util {
	template<typename T>
	struct V8CallbackData {
		CefRefPtr<CefV8Context> context;
		CefRefPtr<CefV8Value> promise;
		T callback;
	};

	template<typename T>
	struct RawCallbackData {
		T callback;
	};
} // namespace Extendify::api::util
