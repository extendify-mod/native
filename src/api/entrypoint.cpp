#include "entrypoint.hpp"

#include "api.hpp"
#include "hook/hook.hpp"

#include <capi/cef_app_capi.h>
using namespace Extendify;
using namespace api;
using namespace entrypoint;

#define f_ret int
#define f_args                                                                                   \
	const cef_main_args_t *args, const struct _cef_settings_t *settings, cef_app_t *application, \
		void *windows_sandbox_info

static f_ret (*origFunc)(f_args) = cef_initialize;

static f_ret hookFunc(f_args) {

	logger.trace("in cef_initialize");
	logger.flush();
	return origFunc(args, settings, application, windows_sandbox_info);
}

int entrypoint::init() {
	logger.trace("hooking function");
	hook::hookFunction(&origFunc, (void*)hookFunc);
	logger.trace("hooked function");
	return 0;
}

int entrypoint::cleanup() {
	throw std::runtime_error("Not implemented");
}
