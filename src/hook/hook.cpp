#include "hook.hpp"

#include <string>

namespace Extendify::hook {
	log::Logger logger({"Extendify", "hook"});
} // namespace Extendify::hook

#ifdef _WIN32

#include <windows.h>

#include <detours.h>
#include <processthreadsapi.h>
#include <winerror.h>

using namespace Extendify;

long hook::hookFunction(void* orig, void* hookFunc) {
	std::string msg;
	bool abort = false;
	long ret;
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
	abort = true;
	ret = DetourUpdateThread(GetCurrentThread());
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_NOT_ENOUGH_MEMORY:
				msg = "ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available "
					  "to complete the operation.";
				goto err;
			default:
				msg = "Unknown error: " + std::to_string(ret);
				goto err;
		}
	}
	ret = DetourAttach((void**)orig, hookFunc);
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_INVALID_BLOCK:
				msg = "ERROR_INVALID_BLOCK: The specified function is too "
					  "small to be hooked.";
				goto err;
			case ERROR_INVALID_HANDLE:
				msg = "ERROR_INVALID_HANDLE: The original function is null or "
					  "points to a null pointer.";
				goto err;
			case ERROR_INVALID_OPERATION:
				msg = "ERROR_INVALID_OPERATION: No pending transaction exists.";
				goto err;
			case ERROR_NOT_ENOUGH_MEMORY:
				msg = "ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available "
					  "to complete the operation.";
				goto err;
			default:
				msg = "Unknown error: " + std::to_string(ret);
				goto err;
		}
	}
	ret = DetourTransactionCommit();
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_INVALID_OPERATION:
				msg = "ERROR_INVALID_OPERATION: No pending transaction exists.";
				goto err;
			case ERROR_INVALID_DATA:
				msg = "ERROR_INVALID_DATA: Target function was changed by "
					  "third party bewteen steps of the transaction.";
				goto err;
			default:
				msg = "Unknown error: " + std::to_string(ret);
				goto err;
		}
	}
	return 0;
err:
	logger.error("Error while hooking function: {}", msg);
	if (abort) {
		logger.debug("Trying to abort errored transaction");
		auto c = DetourTransactionAbort();
		if (c != NO_ERROR) {
			logger.error("Error while aborting transaction: {}", c);
		}
	}
	return ret;
}

long hook::unhookFunction(void* orig, void* hookFunc) {
#warning TODO
	throw std::runtime_error("Unhooking function is not implemented");
}

#elif defined(__linux__)

#include "hook.hpp"

#include <dobby.h>

namespace Extendify::hook {
	long hookFunction(void* orig, void* hookFunc) {
		if (const long ret = DobbyHook(*static_cast<void**>(orig),
									   hookFunc,
									   static_cast<void**>(orig))) {
			logger.error("Failed to hook function {}", orig);
			return ret;
		}
		return 0;
	}

	long unhookFunction(void* orig, void* hookFunc) {
		if (const long ret = DobbyDestroy(orig)) {
			logger.error("Failed to unhook function {}", orig);
			return ret;
		}
		return 0;
	}
} // namespace Extendify::hook

#endif
