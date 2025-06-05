#pragma once

#include "log/Logger.hpp"

#include <cef_command_line.h>
#include <cef_process_message.h>
#include <expected>
#include <filesystem>
#include <internal/cef_types.h>
#include <string>

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
		}
		if (typeFlag == "renderer") {
			return processId == PID_RENDERER;
		}
		return false;
	};

	namespace {
		template<typename T>
		struct _into {
			template<typename... Args>
			constexpr static T operator()(Args&&... values) {
				return T(std::forward<Args>(values)...);
			}
		};

		template<>
		struct _into<std::string> {
			using T = std::string;

			constexpr static T operator()(std::filesystem::path& value) {
				return value.string();
			}

			template<typename... Args>
			constexpr static T operator()(Args&&... values) {
				return T(std::forward<Args>(values)...);
			}
		};
	} // namespace

	template<typename T>
	const auto into = [](auto&&... values) {
		return _into<T>::operator()(std::forward<decltype(values)>(values)...);
	};

	// cursed that theres not a method on std::expected to do this
	template<typename T, typename E>
	auto mergeResult(std::expected<T, E> result) {
		return result.value_or(result.error());
	}
} // namespace Extendify::util
