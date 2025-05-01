#include "hook.hpp"

#include <windows.h>

#include <detours.h>
#include <string>
#include <winerror.h>

using namespace Extendify;

long hook::hookFunction(void* orig, void* hookFunc) {
	std::string msg;
	long ret = DetourTransactionBegin();
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_INVALID_OPERATION:
				msg = "ERROR_INVALID_OPERATION: A pending transaction already "
					  "exists.";
				goto err;
			default:
				msg = "Unknown error: " + std::to_string(ret);
				goto err;
		}
	}
	return 0;
err:
	hook::logger.error("Error while hooking function: {}", msg);
	return ret;
}
