#include "hook.hpp"

#include <windows.h>

#include <detours.h>
#include <string>
#include <winerror.h>

long Extendify::hook::hookFunction(void* orig, void* hookFunc) {
	long ret;
	std::string msg;
	ret = DetourTransactionBegin();
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
err:

}
