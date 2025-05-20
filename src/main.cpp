#include "api/entrypoint.hpp"
#include "log/log.hpp"

#include <cef_version_info.h>
#include <cstdio>
#include <iostream>

#ifdef _WIN32
#include <windows.h>

#include <consoleapi.h>

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
		case DLL_THREAD_DETACH:
			ret = runStop();
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
