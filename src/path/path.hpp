#pragma once
#include "log/Logger.hpp"

#include <filesystem>

namespace Extendify::path {
	extern log::Logger logger;
	std::filesystem::path getBaseConfigDir();
	std::filesystem::path getBaseConfigDir(bool createIfNeeded);
	std::filesystem::path getConfigFilePath();
	std::filesystem::path getConfigFilePath(bool createIfNeeded);
	std::filesystem::path getLogDir();
	std::filesystem::path getLogDir(bool createIfNeeded);
}
