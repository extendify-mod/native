#include "jsInjection.hpp"

#include "api.hpp"
#include "hook/hook.hpp"

#include <capi/cef_v8_capi.h>
#include <cef_callback.h> // needed because of error in cef_v8.h, must come before it
#include <cef_v8.h>
#include <cstddef>
#include <internal/cef_string.h>
#include <internal/cef_string_wrappers.h>

#define f_ret  cef_v8_value_t*
#define f_args const cef_string_t *name, cef_v8_handler_t *handler

static const CefStringUTF16 windowFuncName = "sendCosmosRequest";

namespace Extendify::api::jsInjection {
	namespace {
		f_ret (*origFunc)(f_args) = cef_v8_value_create_function;

		f_ret hookFunc(f_args) {
			logger.trace("in cef_v8_value_create_function");
			CefStringUTF16 nameStr;
			nameStr.FromString16(name->str, name->length);
			if (nameStr == windowFuncName) {
				logger.info("Found window context using function {}, injecting "
							"native API",
							nameStr.ToString());
				api::inject(CefV8Context::GetEnteredContext());
			}
			logger.debug("function name is {}", nameStr.ToString());
			logger.debug("function handler is {:#10x}", (size_t)handler);
			return origFunc(name, handler);
		}
	} // namespace

	void init() {
		hook::hookFunction(static_cast<void*>(&origFunc), (void*)hookFunc);
	}

	void cleanup() {
		static volatile std::atomic_bool cleanupCalled = false;
		if (cleanupCalled.exchange(true)) {
			logger.warn("jsInjection::cleanup already called, ignoring");
			return;
		}
		hook::unhookFunction((void*)&origFunc, (void*)hookFunc);
	}

} // namespace Extendify::api::jsInjection
