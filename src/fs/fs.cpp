#include "fs.hpp"

#include <cef_command_line.h>
#include <cef_process_util.h>
#include <cef_task.h>
#include <cef_thread.h>
#include <filesystem>
#include <fstream>
#include <internal/cef_types.h>
#include <sstream>

#ifdef _WIN32
#include "log/log.hpp"

#include <objbase.h>
#include <processenv.h>
#include <winerror.h>

#endif

namespace Extendify::fs {
	log::Logger logger({"Extendify", "fs"});

	std::string readFile(const std::filesystem::path& path) {
		if (!std::filesystem::exists(path)) {
			logger.warn("Attempting to read a file that does not exist: {}",
						path.string());
			return "";
		}

		std::ifstream fileStream(path);
		std::ostringstream ret;
		ret << fileStream.rdbuf();

		return ret.str();
	}

	void writeFile(const std::filesystem::path& path,
				   const std::string& contents) {
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
		bool XDGOpen(const std::filesystem::path& file) {
			auto cmd = CefCommandLine::CreateCommandLine();
			cmd->SetProgram("xdg-open");
			cmd->AppendArgument(file.string());
			CefTaskRunner::GetForThread(CefThreadId::TID_PROCESS_LAUNCHER)
				->PostTask(util::TaskCBHandler::Create([cmd]() {
					//
					CefLaunchProcess(cmd);
				}));
			return false;
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

		constexpr bool
		winHasExecutableExtension(const std::filesystem::path& file) {
			return file.has_extension()
				   && winIsExecutableExtension(file.extension().string());
		}

		constexpr std::string getShellExecuteError(INT_PTR err) {
			switch (err) {
				case 0:
					return "The operating system is out of memory or "
						   "resources.";
				// SE_ERR_FNF == ERROR_FILE_NOT_FOUND
				// case SE_ERR_FNF:
				case ERROR_FILE_NOT_FOUND:
					return "The specified file was not found.";
				// SE_ERR_PNF == ERROR_PATH_NOT_FOUND
				// case SE_ERR_PNF:
				case ERROR_PATH_NOT_FOUND:
					return "The specified path was not found.";
				case ERROR_BAD_FORMAT:
					return "The .exe file is invalid (non-Win32 .exe or error "
						   "in .exe image).";
				case SE_ERR_ACCESSDENIED:
					return "The operating system denied access to the "
						   "specified file.";
				case SE_ERR_ASSOCINCOMPLETE:
					return "The file name association is incomplete or "
						   "invalid.";
				case SE_ERR_DDEBUSY:
					return "The DDE transaction could not be completed because "
						   "other DDE "
						   "transactions were being processed.";
				case SE_ERR_DDEFAIL:
					return "The DDE transaction failed.";
				case SE_ERR_DDETIMEOUT:
					return "The DDE transaction could not be completed because "
						   "the request "
						   "timed out.";
				case SE_ERR_DLLNOTFOUND:
					return "The specified DLL was not found.";

				case SE_ERR_NOASSOC:
					return "There is no application associated with the given "
						   "filename "
						   "extension.";
				case SE_ERR_OOM:
					return "There was not enough memory to complete the "
						   "operation.";
				case SE_ERR_SHARE:
					return "A sharing violation occurred.";
				default:
					E_ASSERT(false && "Unknown error code");
			}
		}

		/**
		 * @brief Opens a file in Windows.
		 *
		 * @param file The file to open.
		 * @return false if the file was opened successfully, true otherwise.
		 */
		void openPathWin(const std::filesystem::path& file) {
			if (winHasExecutableExtension(file)) [[unlikely]] {
				logger.warn("Attempting to open an executable file: {}",
							file.string());
				return;
			}
			// https://github.com/microsoft/vscode-cpptools/issues/13644
			// NOLINTNEXTLINE(concurrency-mt-unsafe)
			if (std::getenv("ELECTRON_RUN_AS_NODE") != nullptr) {
				SetEnvironmentVariableA("ELECTRON_RUN_AS_NODE", nullptr);
			}
			// https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew#return-value
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			INT_PTR ret = reinterpret_cast<INT_PTR>(
				ShellExecuteW(nullptr,
							  nullptr,
							  file.wstring().c_str(),
							  nullptr,
							  file.parent_path().wstring().c_str(),
							  SW_SHOWNORMAL));
			if (ret <= 32) {
				logger.error("ShellExecute failed: {}",
							 getShellExecuteError(ret));
			} else {
				logger.debug("done");
			}
		}
#endif
	} // namespace

	void openPath(const std::filesystem::path& path) {
#ifdef __linux__
		XDGOpen(path);
#elif defined(_WIN32)
		openPathWin(path);
#endif
	};

} // namespace Extendify::fs
