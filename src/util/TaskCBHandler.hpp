#pragma once
#include <cef_base.h>
#include <cef_task.h>
#include <functional>


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
