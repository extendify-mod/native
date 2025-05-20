#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *ENV_VAR_NAME = "EXTENDIFY_LIB_PATH";

static const char *SPOTIFY_NAME = ".spotify-wrapped";

int find_me_test() {
	fprintf(stderr, "find_me_test called\n");
	int foo = 0;
	foo +=2;
	return foo;
}

int __attribute__((constructor)) start() {
	const char *path = getenv(ENV_VAR_NAME);
	if (path == NULL) {
		fprintf(stderr, "Environment variable %s not set\n", ENV_VAR_NAME);
		return 1;
	}
	printf("%d", find_me_test());
	FILE *fd = fopen("/proc/self/cmdline", "r");
	if (fd == NULL) {
		fprintf(stderr, "Failed to open /proc/self/cmdline");
		return 1;
	}

	char cmdline[4096];

	ssize_t len = fread(cmdline, sizeof(char), sizeof(cmdline) - 1, fd);

	cmdline[len] = '\0';

	fclose(fd);

	if (strcmp(cmdline, "/proc/self/exe") == 0) {
		ssize_t numRead = readlink("/proc/self/exe", cmdline, 4096);
		if (numRead == -1) {
			fprintf(stderr, "Failed to read /proc/self/exe\n");
			return 1;
		}
		if (numRead == 4096) {
			fprintf(stderr, "Path too long\n");
			return 1;
		}
	}

	if (strstr(cmdline, SPOTIFY_NAME) != NULL) {
		fprintf(stderr, "Command line contains %s\n", SPOTIFY_NAME);
		fprintf(stderr, "Loading libextendify.so from %s\n", path);
		void* ret = dlopen(path, RTLD_NOW);
		if (!ret) {
			fprintf(stderr, "Failed to load libextendify.so: %s\n", dlerror());
			return 1;
		} else {
			fprintf(stderr, "Loaded libextendify.so\n");
		}
	} else {
		fprintf(stderr, "Command line does not contain %s\nNot loading libextendify.\n",
				SPOTIFY_NAME);
		fprintf(stderr, "cmdline is: %s\n", cmdline);
	}

	return 0;
}

int __attribute__((destructor)) stop() {
	return 0;
}