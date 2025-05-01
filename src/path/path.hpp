#pragma once
#include <filesystem>

namespace Extendify::path {
	std::filesystem::path getBaseConfigDir();
	std::filesystem::path getBaseConfigDir(bool createIfNeeded);
	std::filesystem::path getConfigFilePath();
	std::filesystem::path getConfigFilePath(bool createIfNeeded);
	std::filesystem::path getLogDir();
	std::filesystem::path getLogDir(bool createIfNeeded);
}
