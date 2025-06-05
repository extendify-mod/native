#include "main.hpp"

#include "api/entrypoint.hpp"
#include "log/Logger.hpp"

#include <cef_version_info.h>
#include <exception>

#ifdef _WIN32
#include <minwindef.h>
#include <winnt.h>
#endif

namespace Extendify {
	log::Logger logger({"Extendify"});

	InitStatus runStart() {
		try {
			api::entrypoint::init();
		} catch (const std::exception& e) {
			return InitStatus::ERR;
		}
		return InitStatus::OK;
	}

	InitStatus runStop() {
		return InitStatus::OK;
	}
} // namespace Extendify

#if defined(_WIN32)
// NOLINTNEXTLINE(misc-use-internal-linkage)
BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD fdwReason,
					LPVOID /*lpvReserved*/) {
	Extendify::InitStatus ret = Extendify::InitStatus::NOT_STARTED;
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			ret = Extendify::runStart();
			break;
		case DLL_THREAD_DETACH:
			ret = Extendify::runStop();
		case DLL_THREAD_ATTACH:
		case DLL_PROCESS_DETACH:
		default:
			break;
	}
	return TRUE;
}
#elif defined(__linux__)
int __attribute__((constructor)) ctor() {
	const Status ret = Extendify::runStart();
	if (ret != OK) {
		Extendify::logger.error("Failed to runStart");
	}
	return 0;
}

int __attribute__((destructor)) dtor() {
	const Status ret = Extendify::runStop();
	if (ret != OK) {
		Extendify::logger.error("Failed to runStop");
	}
	return 0;
}
#endif
