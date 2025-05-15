#include <windows.h>
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* ENV_VAR_NAME = "EXTENDIFY_LIB_PATH";

static const char* SPOTIFY_NAME = "Spotify.exe";

VOID CALLBACK DetourFinishHelperProcess(_In_ HWND _a,
                                        _In_ HINSTANCE _b,
                                        _In_ LPSTR _c,
                                        _In_ INT _d) {

};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason != DLL_PROCESS_ATTACH)
		return TRUE;
	const char* _path = getenv(ENV_VAR_NAME);
	if (_path == NULL) {
		fprintf(stderr, "Environment variable %s not set\n", ENV_VAR_NAME);
		return 1;
	}
	char* path = malloc(strlen(_path));
	if (path == NULL) {
		fprintf(stderr, "Malloc Failed");
		return 1;
	}
	memcpy(path, _path, strlen(_path));

	for (char* c = path; *c; c++) {
		*c = tolower(*c); // NOLINT(*-narrowing-conversions)
	}

	char* cmdline = GetCommandLineA();

	if (strstr(cmdline, SPOTIFY_NAME) != NULL) {
		fprintf(stderr, "Command line contains %s\n", SPOTIFY_NAME);
		fprintf(stderr, "Loading libsadan.so from %s\n", path);
		LoadLibraryA(path);
	} else {
		fprintf(stderr, "Command line does not contain %s\nNot loading libsadan.\n",
		        SPOTIFY_NAME);
		fprintf(stderr, "cmdline is: %s\n", cmdline);
	}
	free(path);
	return 0;
}
