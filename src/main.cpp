#include "main.hpp"

#include "api/entrypoint.hpp"

#include <cef_version_info.h>

#ifdef _WIN32
#include <windows.h>
#endif

enum Status {
	NOT_STARTED,
	OK,
	ERR
};

namespace Extendify {
	log::Logger logger({"Extendify"});

	Status runStart() {
		logger.trace("runStart started");

		try {
			api::entrypoint::init();
		} catch (const std::exception& e) {
			logger.error("Failed to runStart: {}", e.what());
			return ERR;
		}
		logger.trace("runStart finished");
		return OK;
	}

	Status runStop() {
		return OK;
	}
} // namespace Extendify

#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	Status ret = Status::NOT_STARTED;
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
	std::cout << "Extendify: ctor" << std::endl;
	const Status ret = runStart();
	if (ret != OK) {
		Extendify::logger.error("Failed to runStart");
	}
	return 0;
}

int __attribute__((destructor)) dtor() {
	const Status ret = runStop();
	if (ret != OK) {
		Extendify::logger.error("Failed to runStop");
	}
	return 0;
}
#endif
