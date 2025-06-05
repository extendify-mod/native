#pragma once
#include "api/util/V8CallbackData.hpp"

#include <atomic>
#include <cef_thread.h>
#include <cef_v8.h>
#include <cstdint>
#include <expected>
#include <functional>
#include <internal/cef_ptr.h>
#include <memory>
#include <string>
#include <variant>

namespace Extendify::util {
	// because windows defines a macro called MessageBox

	class MsgBox {
	  public:
		enum class Type : uint8_t {
			OK,
			YES_NO,
			OK_CANCEL,
		};
		enum class Result : uint8_t {
			OK, // same as YES
			CANCEL, // same as NO
			ERR // an error occurred while showing the message box
		};
		using Callback = std::function<void(std::shared_ptr<MsgBox>, Result)>;
		using PromiseCallback =
			std::function<std::expected<CefRefPtr<CefV8Value>, std::string>(
				std::shared_ptr<MsgBox>, Result)>;

		MsgBox() = default;

		[[nodiscard]] static std::shared_ptr<MsgBox>
		Create(std::string title, std::string message, Type type);
		[[nodiscard]] CefRefPtr<CefV8Value>
		promise(CefRefPtr<CefV8Context> context, PromiseCallback callback);
		void launch(Callback callback);
		[[nodiscard]] Result show() const;

		[[nodiscard]] constexpr bool isRunning() const noexcept {
			return running;
		}

	  private:
		std::atomic_bool running = false;
		std::string title = "MsgBox";
		std::string message = "Message";
		Type type = Type::OK;
		CefRefPtr<CefThread> thread;
		std::shared_ptr<MsgBox> self;
		std::variant<api::util::V8CallbackData<PromiseCallback>,
					 api::util::RawCallbackData<Callback>>
			data;
		void runThread();

		[[nodiscard]] Result run() const;
	};
} // namespace Extendify::util
