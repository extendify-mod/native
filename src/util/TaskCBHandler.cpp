#include "TaskCBHandler.hpp"

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
		assert(handler);
		handler();
	}

} // namespace Extendify::util
