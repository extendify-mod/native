#include "entrypoint.hpp"

#include "api.hpp"
#include "jsInjection.hpp"
#include "hook/hook.hpp"

#include <capi/cef_app_capi.h>
#include <include/internal/cef_types.h>

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
	if(!settings->remote_debugging_port) {
		logger.info("remote debugging port is not set, setting to 9229");
		const_cast<_cef_settings_t*>(settings)->remote_debugging_port = 9229;
	} else {
		logger.info("remote debugging port is set to {}", settings->remote_debugging_port);
	}
	return origFunc(args, settings, application, windows_sandbox_info);
}

int entrypoint::init() {
	hook::hookFunction(&origFunc, (void*)hookFunc);
	jsInjection::init();
	return 0;
}

int entrypoint::cleanup() {
	throw std::runtime_error("Not implemented");
}
