#include "log/log.hpp"
#include <cef_version_info.h>

#ifdef _WIN32
#include <windows.h>
#endif

enum Status {
	NOT_STARTED,
	OK,
	ERR
};

Status runStart() {
	return ERR;
}

Status runStop() {
	return OK;
}

#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	Status ret = Status::NOT_STARTED;
	cef_version_info(0);
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
