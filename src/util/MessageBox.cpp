#include "MessageBox.hpp"

#include "api/util/ScopedV8Context.hpp"
#include "log/log.hpp"
#include "string.hpp"
#include "util/TaskCBHandler.hpp"
#include "util/util.hpp"

#include <internal/cef_ptr.h>
#include <internal/cef_types.h>
#include <variant>

namespace Extendify::util {
	namespace {
#ifdef _WIN32
		UINT getMessageBoxType(MsgBox::Type type) {
			switch (type) {
				case MsgBox::Type::OK:
					return MB_OK;
				case MsgBox::Type::YES_NO:
					return MB_YESNO;
				case MsgBox::Type::OK_CANCEL:
					return MB_OKCANCEL;
				default:
					E_ASSERT(false && "Unknown MsgBox type");
			}
		}
#endif
	} // namespace

	// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ensureNotRunning()                                     \
	if (isRunning()) {                                         \
		constexpr auto msg =                                   \
			"Trying to launch a message box while another is " \
			"already running for the same object";             \
		logger.error(msg);                                     \
		throw std::runtime_error(msg);                         \
	}

	[[nodiscard]] CefRefPtr<CefV8Value>
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
	MsgBox::promise(CefRefPtr<CefV8Context> _context,
					PromiseCallback callback) {
		ensureNotRunning();
		running = true;
		api::util::ScopedV8Context _(_context);
		auto promise = CefV8Value::CreatePromise();
		data = api::util::V8CallbackData<PromiseCallback> {
			.context = _context,
			.promise = promise,
			.callback = std::move(callback),
		};
		runThread();
		return promise;
	}

	[[nodiscard]] MsgBox::Result MsgBox::show() const {
		return run();
	}

	void MsgBox::launch(Callback callback) {
		ensureNotRunning();
		running = true;

		data = api::util::RawCallbackData<Callback> {
			.callback = std::move(callback),
		};
		runThread();
	}

	void MsgBox::runThread() {
		if (!thread || !thread->IsRunning()) {
			thread = CefThread::CreateThread("MessageBoxWorker");
		}
		thread->GetTaskRunner()->PostTask(util::TaskCBHandler::Create([this] {
			auto res = run();
			if (std::holds_alternative<
					api::util::V8CallbackData<PromiseCallback>>(data)) {
				CefTaskRunner::GetForThread(TID_RENDERER)
					->PostTask(util::TaskCBHandler::Create([this, res] {
						const auto& cbData = std::get<
							api::util::V8CallbackData<PromiseCallback>>(data);
						E_ASSERT(cbData.context && cbData.promise
								 && cbData.callback
								 && "context or promise or callback is null");
						cbData.context->GetTaskRunner()->PostTask(
							util::TaskCBHandler::Create([this,
														 cbData = cbData,
														 res] {
								api::util::ScopedV8Context _(cbData.context);
								auto _1 = self;
								try {
									auto ret =
										cbData.callback(std::move(self), res);
									if (ret.has_value()) {
										cbData.promise->ResolvePromise(
											std::move(ret.value()));
									} else {
										cbData.promise->RejectPromise(
											ret.error());
									}
								} catch (const std::exception& e) {
									logger.error("Error in MessageBox promise "
												 "callback: {}",
												 e.what());
								}
								running = false;
							}));
					}));
			} else {
				auto& cbData =
					std::get<api::util::RawCallbackData<Callback>>(data);
				auto _ = self;
				try {
					cbData.callback(std::move(self), res);
				} catch (const std::exception& e) {
					logger.error("Error in MessageBox callback: {}", e.what());
				}
				running = false;
			}
		}));
	}

	[[nodiscard]] MsgBox::Result MsgBox::run() const {
#ifdef _WIN32
		const auto titlew = string::stringToWstring(title);
		const auto messagew = string::stringToWstring(message);
		auto ret = MessageBoxW(
			nullptr, messagew.c_str(), titlew.c_str(), getMessageBoxType(type));
		if (!ret) {
			logger.error("Failed to show message box: {}", GetLastError());
			return Result::ERR;
		}
		// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messageboxw#return-value
		if (ret == IDOK || ret == IDYES) {
			return Result::OK; // IDOK and IDYES are the same
		}
		if (ret == IDCANCEL || ret == IDNO) {
			return Result::CANCEL; // IDCANCEL and IDNO are the same
		}

		logger.warn("Unknown return value from MessageBox: {}", ret);
		return Result::ERR;
#elif defined(__linux__)
#warn "Linux support is not implemented yet"
		return Result::ERR;
#endif
	}

	std::shared_ptr<MsgBox> MsgBox::Create(std::string title,
										   std::string message, Type type) {
		auto msgBox = std::make_shared<MsgBox>();
		msgBox->title = std::move(title);
		msgBox->message = std::move(message);
		msgBox->type = type;
		msgBox->self = msgBox;
		return msgBox;
	};
} // namespace Extendify::util
