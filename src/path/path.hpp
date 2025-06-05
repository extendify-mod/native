#pragma once
#include "log/Logger.hpp"

#include <filesystem>
#include <string>

namespace Extendify::path {
	extern log::Logger logger;
	std::filesystem::path getBaseConfigDir();
	std::filesystem::path getBaseConfigDir(bool createIfNeeded);
	std::filesystem::path getConfigFilePath();
	std::filesystem::path getConfigFilePath(bool createIfNeeded);
	std::filesystem::path getLogDir();
	std::filesystem::path getLogDir(bool createIfNeeded);
	std::filesystem::path getQuickCssFile();
	std::filesystem::path getQuickCssFile(bool createIfNeeded);
	std::filesystem::path getThemesDir();
	std::filesystem::path getThemesDir(bool createIfNeeded);

	/**
	 * @brief Creates `path` if it does not exist
	 *
	 * @param path the path of the directory to create
	 * @return true if the directory was created, false if it already existed
	 */
	bool ensureDir(const std::filesystem::path& path);

	/**
	 * @brief Creates `path` if it does not exist
	 * @param path the path of the file to create
	 * @return true if the file was created, false otherwise
	 */
	bool ensureFile(const std::filesystem::path& path);

	/**
	 * @brief Creates `path` if it does not exist
	 * @param path the path of the file to create
	 * @param defaultContent The default content to create the file with if it
	 * doesn't exist
	 * @return true if the file was created, false otherwise
	 */
	bool ensureFile(const std::filesystem::path& path,
					const std::string& defaultContent);
} // namespace Extendify::path
