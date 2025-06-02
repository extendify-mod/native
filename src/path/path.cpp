#include "path.hpp"

#include "api/settings.hpp"

#include <filesystem>
#include <fstream>

using namespace Extendify;
namespace fs = std::filesystem;
log::Logger path::logger({"Extendify", "path"});

fs::path path::getBaseConfigDir() {
	return getBaseConfigDir(false);
}

fs::path path::getBaseConfigDir(bool createIfNeeded) {
	std::optional<fs::path> ret;
#ifdef __linux__
	if (const auto xdgConfigEnv = std::getenv("XDG_CONFIG_HOME")) {
		ret = fs::path(xdgConfigEnv) / "extendify";
	} else if (const auto homePath = std::getenv("HOME")) {
		ret = fs::path(homePath) / ".config" / "extendify";
	}
#elifdef _WIN32
	if (const auto appData = std::getenv("APPDATA")) {
		ret = fs::path(appData) / "extendify";
	}
#endif
	if (ret.has_value()) {
		if (createIfNeeded) {
			ensureDir(ret.value());
		}
		return ret.value();
	}
	logger.error("Error getting base path, both $HOME and $XDG_CONFIG_DIR are not set");
	throw std::runtime_error("Error getting base path, both $HOME and $XDG_CONFIG_DIR are not set");
}

fs::path path::getConfigFilePath() {
	return getConfigFilePath(false);
}

std::filesystem::path path::getConfigFilePath(bool createIfNeeded) {
	const auto base = getBaseConfigDir(createIfNeeded);
	auto configFile = base / "config.json";
	if (createIfNeeded) {
		ensureFile(configFile, api::settings::defaultSettingsJSON);
	}
	return configFile;
}

std::filesystem::path path::getLogDir(bool createIfNeeded) {
	const auto base = getBaseConfigDir(createIfNeeded);
	auto logPath = base / "logs";
	if (createIfNeeded) {
		ensureDir(logPath);
	}
	return logPath;
}

fs::path path::getLogDir() {
	return getLogDir(false);
}

fs::path path::getQuickCssFile() {
	return getQuickCssFile(false);
}

fs::path path::getQuickCssFile(bool createIfNeeded) {
	const auto base = getBaseConfigDir(createIfNeeded);
	auto quickCssPath = base / "quickCss.css";
	if (createIfNeeded) {
		ensureFile(quickCssPath);
	}
	return quickCssPath;
}

fs::path path::getThemesDir() {
	return getThemesDir(false);
}

fs::path path::getThemesDir(bool createIfNeeded) {
	const auto base = getBaseConfigDir(createIfNeeded);
	auto themesPath = base / "themes";
	if (createIfNeeded) {
		ensureDir(themesPath);
	}
	return themesPath;
}

bool path::ensureDir(const std::filesystem::path& path) {
	if (fs::exists(path)) {
		logger.debug("Directory {} already exists", path.string());
		return false;
	}
	logger.debug("Creating directory {}", path.string());
	fs::create_directories(path);
	return true;
}

bool path::ensureFile(const std::filesystem::path& path) {
	return ensureFile(path, "");
}

bool path::ensureFile(const std::filesystem::path& path, const std::string& defaultContent) {
	if (fs::exists(path)) {
		logger.debug("File {} already exists", path.string());
		return false;
	}
	logger.debug("Creating file {}", path.string());
	std::ofstream stream(path);
	if (!stream) {
		logger.error("Error creating file {}", path.string());
		throw std::runtime_error("Error creating file " + path.string());
	}
	if (!defaultContent.empty()) {
		stream << defaultContent;
	}
	return true;
}
