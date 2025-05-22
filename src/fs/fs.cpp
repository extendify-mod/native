#include "fs.hpp"

#include <filesystem>
#include <cef_command_line.h>
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

#ifdef __linux__
	namespace {
		// Descriptions pulled from https://linux.die.net/man/1/xdg-open
		std::string GetErrorDescription(int error_code) {
			switch (error_code) {
				case 1:
					return "Error in command line syntax";
				case 2:
					return "The item does not exist";
				case 3:
					return "A required tool could not be found";
				case 4:
					return "The action failed";
				default:
					return "";
			}
		}

		bool XDGUtil(const std::vector<std::string>& argv, const std::filesystem::path& workingDir,
					 const bool waitForExit, const bool focusLaunchedProcess,
					 const OpenCallback& callback) {
			auto cli = CefCommandLine::CreateCommandLine();
			#warning TODO
			return false;
		}

		bool XDGOpen(const std::filesystem::path& working_dir, const std::string& fileName,
					 const bool waitForExit, const OpenCallback& callback) {
			return XDGUtil({"xdg-open", fileName}, working_dir, waitForExit, true, callback);
		}
	} // namespace
#endif

	void openPath(const std::filesystem::path& path, const OpenCallback& callback) {
		#warning todo
	};
} // namespace Extendify::fs
