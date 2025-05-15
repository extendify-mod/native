#include "log/log.hpp"
#include "api/entrypoint.hpp"
#include <cef_version_info.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Extendify;

enum Status {
	NOT_STARTED,
	OK,
	ERR
};

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

#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	Status ret = Status::NOT_STARTED;
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			ret = runStart();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			ret = runStop();
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
#elif defined(__linux__)
__attribute__((constructor)) void ctor() {
	const Status ret = runStart();
	if (ret != OK) {
		Extendify::logger.error("Failed to runStart");
	}
}

__attribute__((constructor)) void dtor() {
	const Status ret = runStop();
	if (ret != OK) {
		Extendify::logger.error("Failed to runStop");
	}
	return;
}
#endif
