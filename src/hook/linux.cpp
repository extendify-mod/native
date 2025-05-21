#ifdef __linux__
#include "hook.hpp"

#include <dobby.h>

namespace Extendify::hook {
	long hookFunction(void* orig, void* hookFunc) {
		if (const long ret = DobbyHook(*static_cast<void**>(orig), hookFunc, static_cast<void**>(orig))) {
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
