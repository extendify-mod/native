#include "ScopedV8Context.hpp"

#include "log/log.hpp"
#include "log/Logger.hpp"

#include <cef_v8.h>
#include <internal/cef_ptr.h>
#include <utility>


namespace Extendify::api::util {
	ScopedV8Context::ScopedV8Context(CefRefPtr<CefV8Context> _context):
		id(nextId++),
		context(std::move(_context)) {
		if (!context->IsValid()) {
			E_ASSERT(
				false
				&& "trying to use ScopedV8Context with an invalid context");
		}
		if (CefV8Context::InContext()
			&& CefV8Context::GetEnteredContext()->IsSame(context)) {
			shouldExit = false;
		} else {
			context->Enter();
		}
		logger.trace(
			"Entering ScopedV8Context {}, shouldExit: {}", id, shouldExit);
	}

	ScopedV8Context::~ScopedV8Context() {
		if (shouldExit) {
			context->Exit();
		}
		logger.trace(
			"Exiting ScopedV8Context {}, shouldExit: {}", id, shouldExit);
	}

	log::Logger ScopedV8Context::logger {
		{"Extendify", "api", "ScopedV8Context"}};
	int ScopedV8Context::nextId = 1;
} // namespace Extendify::api::util
