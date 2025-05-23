#include "TaskCBHandler.hpp"
#include "log/log.hpp"

#include <utility>

namespace Extendify::util {

	CefRefPtr<TaskCBHandler> TaskCBHandler::Create(std::function<void()> func) {
		CefRefPtr<TaskCBHandler> handler = new TaskCBHandler();
		handler->setCallback(std::move(func));
		return handler;
	}

	void TaskCBHandler::setCallback(std::function<void()> func) {
		handler = std::move(func);
	}

	void TaskCBHandler::Execute() {
		E_ASSERT(handler);
		handler();
	}

} // namespace Extendify::util
