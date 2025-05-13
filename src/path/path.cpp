#include "path.hpp"

#include <filesystem>

using namespace Extendify;
namespace fs = std::filesystem;
log::Logger path::logger({"Extendify", "path"});

fs::path path::getBaseConfigDir() {
	return getBaseConfigDir(false);
}

fs::path path::getBaseConfigDir(bool createIfNeeded) {
	std::optional<fs::path> ret;
	if (const auto xdgConfigEnv = std::getenv("XDG_CONFIG_HOME")) {
		ret = fs::path(xdgConfigEnv) / "extendify";
	}
	if (const auto homePath = std::getenv("HOME")) {
		ret = fs::path(homePath) / ".config" / "extendify";
	}
ensureExists:
	logger.error("Error getting base path, both $HOME and $XDG_CONFIG_DIR are not set");
	throw std::runtime_error("Error getting base path, both $HOME and $XDG_CONFIG_DIR are not set");

}

fs::path path::getConfigFilePath() {
	return getConfigFilePath(false);
}

fs::path path::getLogDir() {
	return getLogDir(false);
}
