#pragma once

#include "log/Logger.hpp"

#include <filesystem>
#include <string>

namespace Extendify::fs {
	extern log::Logger logger;

	/**
	 * @brief reads a file at path and returns its contents as a string
	 *
	 * assumes utf-8 encoding
	 *
	 * @param path the file path
	 * @return std::string the contents of the file or an empty string if the
	 * file does not exist
	 */
	std::string readFile(const std::filesystem::path& path);

	void writeFile(const std::filesystem::path& path,
				   const std::string& contents);

	/**
	 * @brief opens a path in the users default application
	 *
	 * @param path the path to open
	 */
	void openPath(const std::filesystem::path& path);
	
	void ensureFilesCanOpenInVscode();
} // namespace Extendify::fs
