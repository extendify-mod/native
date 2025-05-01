#include "path.hpp"

#include <filesystem>

using namespace Extendify;

std::filesystem::path path::getBaseConfigDir() {
	return path::getBaseConfigDir(false);
}

std::filesystem::path path::getBaseConfigDir(bool createIfNeeded) {
	char* xdgConfigEnv = std::getenv("XDG_CONFIG_HOME");
	if (xdgConfigEnv) {
		return std::filesystem::path(xdgConfigEnv) / "extendify";
	}
	char* homePath = std::getenv("HOME");
	if (homePath) {
		return std::filesystem::path(homePath) / ".config" / "extendify";
	}

}

std::filesystem::path path::getConfigFilePath() {
	return path::getConfigFilePath(false);
}

std::filesystem::path path::getLogDir() {
	return path::getLogDir(false);
}
