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
	bool isTransactionToAbort = false;
	long ret {};
	auto handleError = [&isTransactionToAbort, &msg, &ret]() {
		logger.error("Error while hooking function: {}", msg);
		if (isTransactionToAbort) {
			logger.debug("Trying to abort errored transaction");
			auto abortErrCode = DetourTransactionAbort();
			if (abortErrCode != NO_ERROR) {
				logger.error("Error while aborting transaction: {}",
							 abortErrCode);
			}
		}
		return ret;
	};
	ret = DetourTransactionBegin();
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_INVALID_OPERATION: {
				msg = "ERROR_INVALID_OPERATION: A pending transaction already "
					  "exists.";
				break;
			}
			default:
				msg = "Unknown error: " + std::to_string(ret);
		}
		return handleError();
	}
	isTransactionToAbort = true;
	ret = DetourUpdateThread(GetCurrentThread());
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_NOT_ENOUGH_MEMORY: {
				msg = "ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available "
					  "to complete the operation.";
				break;
			}
			default:
				msg = "Unknown error: " + std::to_string(ret);
		}
		return handleError();
	}
	ret = DetourAttach((void**)orig, hookFunc);
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_INVALID_BLOCK: {
				msg = "ERROR_INVALID_BLOCK: The specified function is too "
					  "small to be hooked.";
				break;
			}
			case ERROR_INVALID_HANDLE: {
				msg = "ERROR_INVALID_HANDLE: The original function is null or "
					  "points to a null pointer.";
				break;
			}
			case ERROR_INVALID_OPERATION: {
				msg = "ERROR_INVALID_OPERATION: No pending transaction exists.";
				break;
			}
			case ERROR_NOT_ENOUGH_MEMORY: {
				msg = "ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available "
					  "to complete the operation.";
				break;
			}
			default:
				msg = "Unknown error: " + std::to_string(ret);
		}
		return handleError();
	}
	ret = DetourTransactionCommit();
	if (ret != NO_ERROR) {
		switch (ret) {
			case ERROR_INVALID_OPERATION: {
				msg = "ERROR_INVALID_OPERATION: No pending transaction exists.";
				break;
			}
			case ERROR_INVALID_DATA: {
				msg = "ERROR_INVALID_DATA: Target function was changed by "
					  "third party bewteen steps of the transaction.";
				break;
			}
			default:
				msg = "Unknown error: " + std::to_string(ret);
		}
		return handleError();
	}
	return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
long hook::unhookFunction(void* /*orig*/, void* /*hookFunc*/) {
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
