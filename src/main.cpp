#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

enum Status {
  NOT_STARTED,
  OK,
// horror
#pragma push_macro("ERROR")
#undef ERROR
  ERROR
#pragma pop_macro("ERROR")

};

Status runStart() {}

Status runStop() {}

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
#endif