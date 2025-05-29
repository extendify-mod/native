#pragma once

#include "log/Logger.hpp"

#include <cef_command_line.h>
#include <include/cef_process_message.h>
#include <include/internal/cef_types.h>

namespace Extendify::util {
	extern log::Logger logger;

	[[nodiscard]] inline bool isOnProcess(CefProcessId processId) noexcept {
		const auto cmdline = CefCommandLine::GetGlobalCommandLine();
		if (cmdline->HasSwitch("--single-process")) {
			return true;
		}

		const auto typeFlag = cmdline->GetSwitchValue("type");
		if (typeFlag.empty()) {
			return processId == PID_BROWSER;
		} else if (typeFlag == "renderer") {
			return processId == PID_RENDERER;
		}
		return false;
	};

	template<typename T>
	struct into {
		template<typename... Args>
		static T operator()(Args&&... args) {
			return T(std::forward<Args>(args)...);
		}
	};
} // namespace Extendify::util
