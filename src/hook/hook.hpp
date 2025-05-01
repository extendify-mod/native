#include "log/Logger.hpp"

namespace Extendify {
	namespace hook {
		extern log::Logger logger;
		long hookFunction(void* orig, void* hookFunc);
		long unhookFunction(void* orig, void* hookFunc);
	} // namespace hook
} // namespace Extendify
