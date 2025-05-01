#pragma once
#include "Logger.hpp"

#include <filesystem>
#include <spdlog/logger.h>


namespace Extendify {
	namespace log {
        std::filesystem::path getLogDir();
	} // namespace log

	extern log::Logger logger();
}; // namespace Extendify
