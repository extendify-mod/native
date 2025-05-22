#pragma once

#include "log/Logger.hpp"

#include <filesystem>


namespace Extendify::fs {
	extern log::Logger logger;

	/**
	 * @brief reads a file at path and returns its contents as a string
	 *
	 * assumes utf-8 encoding
	 *
	 * @param path the file path
	 * @return std::string the contents of the file or an empty string if the file does not exist
	 */
	std::string readFile(const std::filesystem::path& path);

	void writeFile(const std::filesystem::path& path, const std::string& contents);
	
	typedef std::optional<std::function<void(const std::string&)>> OpenCallback;
	/**
	 * @brief opens a path in the users default application
	 * 
	 * @param path 
	 */
	void openPath(const std::filesystem::path& path, OpenCallback callback);
} // namespace Extendify::fs
