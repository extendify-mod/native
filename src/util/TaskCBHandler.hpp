#pragma once
#include "util.hpp"

#include <cef_base.h>
#include <functional>
#include <include/cef_task.h>

namespace Extendify::util {
	class TaskCBHandler: public CefTask {
	  public:
		void Execute() override;
		static CefRefPtr<TaskCBHandler> Create(std::function<void()> func);

	  private:
		std::function<void()> handler;
		void setCallback(std::function<void()> func);
		IMPLEMENT_REFCOUNTING(TaskCBHandler);
	};
} // namespace Extendify::util
