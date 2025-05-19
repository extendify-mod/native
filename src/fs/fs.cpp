#include "fs.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace Extendify::fs {
	log::Logger logger({"Extendify", "fs"});

	std::string readFile(const std::filesystem::path& path) {
		if (!std::filesystem::exists(path)) {
			logger.warn("Attempting to read a file that does not exist: {}", path.string());
			return "";
		}

		std::ifstream fileStream(path);
		std::ostringstream ret;
		ret << fileStream.rdbuf();

		return ret.str();
	}

	void writeFile(const std::filesystem::path& path, const std::string& contents) {
		std::ofstream fileStream(path);
		if (!fileStream) {
			logger.error("Failed to open file for writing: {}", path.string());
            throw std::runtime_error("Failed to open file for writing");
		}
		fileStream << contents;
		if (!fileStream) {
			logger.error("Failed to write contents to file: {}", path.string());
			throw std::runtime_error("Failed to write contents to file");
		}
	}
} // namespace Extendify::fs
