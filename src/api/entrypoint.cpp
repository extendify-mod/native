#include "entrypoint.hpp"

#include "api.hpp"
#include "fs/Watcher.hpp"
#include "hook/hook.hpp"
#include "jsInjection.hpp"

#include <capi/cef_app_capi.h>
#include <include/internal/cef_types.h>

#define f_ret int
#define f_args                                                           \
	const cef_main_args_t *args, const struct _cef_settings_t *settings, \
		cef_app_t *application, void *windows_sandbox_info

namespace Extendify::api::entrypoint {
	namespace {
		f_ret (*origFunc)(f_args) = cef_initialize;

		f_ret hookFunc(f_args) {
			logger.trace("in cef_initialize");
			if (!settings->remote_debugging_port) {
				logger.info(
					"remote debugging port is not set, setting to 9229");
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
				const_cast<_cef_settings_t*>(settings)->remote_debugging_port =
					9229;
			} else {
				logger.info("remote debugging port is set to {}",
							settings->remote_debugging_port);
			}
			int ret =
				origFunc(args, settings, application, windows_sandbox_info);

			logger.info("starting fs watcher");
			fs::Watcher::get()->init();
			logger.info("fs watcher started");
			return ret;
		}
	} // namespace

	int init() {
		hook::hookFunction(static_cast<void*>(&origFunc), (void*)hookFunc);
		jsInjection::init();
		return 0;
	}

	int cleanup() {
		throw std::runtime_error("Not implemented");
	}
} // namespace Extendify::api::entrypoint
