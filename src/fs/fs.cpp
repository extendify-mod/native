#include "fs.hpp"

#include <cef_command_line.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>
#include <winerror.h>

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

	namespace {
#ifdef __linux__
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
#elif defined(_WIN32)
		constexpr bool winIsExecutableExtension(const std::string& ext) {
			static constexpr auto exts = {
				".COM",
				".EXE",
				".BAT",
				".CMD",
				".VBS",
				".VBE",
				".JS",
				".JSE",
				".WSF",
				".WSH",
				".MSC",
			};
			return std::find(exts.begin(), exts.end(), ext) != exts.end();
		}

		constexpr bool winHasExecutableExtension(const std::filesystem::path& file) {
			return file.has_extension() && winIsExecutableExtension(file.extension().string());
		}

		constexpr std::string getShellExecuteError(INT_PTR err) {
			switch (err) {
				case 0:
					return "The operating system is out of memory or resources.";
				// SE_ERR_FNF == ERROR_FILE_NOT_FOUND
				// case SE_ERR_FNF:
				case ERROR_FILE_NOT_FOUND:
					return "The specified file was not found.";
				// SE_ERR_PNF == ERROR_PATH_NOT_FOUND
				// case SE_ERR_PNF:
				case ERROR_PATH_NOT_FOUND:
					return "The specified path was not found.";
				case ERROR_BAD_FORMAT:
					return "The .exe file is invalid (non-Win32 .exe or error in .exe image).";
				case SE_ERR_ACCESSDENIED:
					return "The operating system denied access to the specified file.";
				case SE_ERR_ASSOCINCOMPLETE:
					return "The file name association is incomplete or invalid.";
				case SE_ERR_DDEBUSY:
					return "The DDE transaction could not be completed because other DDE "
						   "transactions were being processed.";
				case SE_ERR_DDEFAIL:
					return "The DDE transaction failed.";
				case SE_ERR_DDETIMEOUT:
					return "The DDE transaction could not be completed because the request "
						   "timed out.";
				case SE_ERR_DLLNOTFOUND:
					return "The specified DLL was not found.";

				case SE_ERR_NOASSOC:
					return "There is no application associated with the given filename "
						   "extension.";
				case SE_ERR_OOM:
					return "There was not enough memory to complete the operation.";
				case SE_ERR_SHARE:
					return "A sharing violation occurred.";
				default:
					assert(false && "Unknown error code");
					std::unreachable();
			}
		}

		/**
		 * @brief Opens a file in Windows.
		 *
		 * @param file The file to open.
		 * @param workingDir The working directory.
		 * @return false if the file was opened successfully, true otherwise.
		 */
		bool openPathWin(const std::filesystem::path& file,
						 std::optional<const std::filesystem::path*> workingDir) {
			if (winHasExecutableExtension(file)) [[unlikely]] {
				logger.warn("Attempting to open an executable file: {}", file.string());
				return true;
			}
			// https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew#return-value
			INT_PTR ret =
				(INT_PTR)ShellExecuteW(nullptr,
									   L"open",
									   file.wstring().c_str(),
									   nullptr,
									   workingDir ? (**workingDir).wstring().c_str() : nullptr,
									   SW_SHOW);
			if (ret <= 32) {
				logger.error("ShellExecute failed: {}", getShellExecuteError(ret));
				return true;
			}
			return false;
		}
#endif
	} // namespace

	bool openPath(const std::filesystem::path& path,
				  std::optional<const std::filesystem::path*> workingDir) {
#ifdef __linux__
#warning todo
#elif defined(_WIN32)
		return openPathWin(path, workingDir);
#endif
	};

} // namespace Extendify::fs
