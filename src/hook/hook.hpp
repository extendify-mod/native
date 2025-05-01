#include "log/log.hpp"

namespace Extendify {
	namespace hook {
		extern log::logger logger;
		long hookFunction(void* orig, void* hookFunc);
		long unhookFunction(void* orig, void* hookFunc);
	} // namespace hook
} // namespace Extendify
