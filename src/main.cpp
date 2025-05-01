#ifdef _WIN32
#include <windows.h>
#endif
#include <cef_version_info.h>
#include <spdlog/spdlog.h>
#include <stdint.h>

enum Status {
	NOT_STARTED,
	OK,
	ERR
};

Status runStart() {
	return OK;
	;
}

Status runStop() {
	return OK;
	;
}

#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	spdlog::log(spdlog::level::err, "TEST");
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
#endif
