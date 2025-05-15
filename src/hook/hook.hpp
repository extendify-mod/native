#include "log/Logger.hpp"

namespace Extendify::hook {
	extern log::Logger logger;

	long hookFunction(void* orig, void* hookFunc);

	long unhookFunction(void* orig, void* hookFunc);
} // namespace Extendify::hook
