#include "jsInjection.hpp"

#include "hook/hook.hpp"

#include <capi/cef_v8_capi.h>
#include <cef_callback.h> // needed because of error in cef_v8.h, must come before it
#include <cef_v8.h>
#include <include/internal/cef_string_wrappers.h>

using namespace Extendify;
using namespace api;
using namespace jsInjection;

#define f_ret  cef_v8_value_t*
#define f_args const cef_string_t *name, cef_v8_handler_t *handler

static f_ret (*origFunc)(f_args) = cef_v8_value_create_function;

static const CefStringUTF16 windowFuncName = "sendCosmosRequest";

static f_ret hookFunc(f_args) {
	logger.trace("in cef_v8_value_create_function");
	CefStringUTF16 nameStr;
	nameStr.FromString16(name->str, name->length);
	if (nameStr == windowFuncName) {
		logger.info("Found window context using function {}, injecting native API", nameStr.ToString());
		api::inject(CefV8Context::GetEnteredContext());
	}
	logger.debug("function name is {}", nameStr.ToString());
	logger.debug("function handler is {}", (size_t)handler);
	return origFunc(name, handler);
}

void jsInjection::init() {
	hook::hookFunction(&origFunc, (void*)hookFunc);
}

void jsInjection::cleanup() {
	logger.error("jsInjection::cleanup not implemented");
}
