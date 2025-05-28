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

	template<typename T, typename... Args>
	[[nodiscard]] inline T into(Args&&... args) {
		return T(std::forward<Args>(args)...);
	}

	namespace {

		template<typename, typename = std::void_t<>>
		struct hasTypeType: std::false_type { };

		template<typename T>
		struct hasTypeType<T, std::void_t<typename T::type>>: std::true_type { };

		template<typename Func, typename... Args>
		struct ReturnType_impl:
			std::conditional<hasTypeType<std::invoke_result<Func, Args...>>::value,
							 typename std::invoke_result<Func, Args...>,
							 typename std::invoke_result<Func>> { };
	} // namespace
    template <typename Func, typename... Args>
    using ReturnType = typename ReturnType_impl<Func, Args...>::type;
} // namespace Extendify::util
