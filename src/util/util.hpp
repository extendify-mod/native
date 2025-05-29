#pragma once

#include "log/Logger.hpp"

#include <cef_command_line.h>
#include <include/cef_process_message.h>
#include <include/internal/cef_types.h>
#include <type_traits>

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
	auto into = [](auto&&... values) {
		return T(std::forward<decltype(values)>(values)...);
	};
	template<typename T>
	std::string into = [](std::filesystem::path&& path) {
		return path.string();
	};

	template<typename T>
	class WrappedPointer {
	  public:
		using pointer = T*;

		// NOLINTNEXTLINE(google-explicit-constructor)
		constexpr WrappedPointer(pointer ptr):
			_ptr(ptr) {
		}

		[[nodiscard]] constexpr typename std::add_lvalue_reference<T>::type
		operator*() const noexcept(noexcept(*std::declval<pointer>())) {
			return *_ptr;
		};

		[[nodiscard]] constexpr pointer operator->() const noexcept {
			return _ptr;
		}
		
		[[nodiscard]] constexpr pointer _get() const noexcept {
			return _ptr;
		}

	  protected:
		pointer _ptr;
	};
} // namespace Extendify::util
