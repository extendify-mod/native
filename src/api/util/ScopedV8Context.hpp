#pragma once

#include "log/Logger.hpp"

#include <cef_v8.h>

namespace Extendify::api::util {
	class ScopedV8Context {
	  public:
		explicit ScopedV8Context(CefRefPtr<CefV8Context> context);

		~ScopedV8Context();

		ScopedV8Context() = delete;
		ScopedV8Context& operator=(const ScopedV8Context&) = delete;
		ScopedV8Context& operator=(ScopedV8Context&&) = delete;
		ScopedV8Context(const ScopedV8Context& other) = delete;
		ScopedV8Context(ScopedV8Context&& other) = delete;

	  private:
		static log::Logger logger;
		static int nextId;
		int id;
		bool shouldExit = true;
		CefRefPtr<CefV8Context> context;
	};
} // namespace Extendify::api::util
