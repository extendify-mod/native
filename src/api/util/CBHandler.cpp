#include "CBHandler.hpp"
#include <internal/cef_ptr.h>
#include <utility>

namespace Extendify::api::util {
	CefRefPtr<CBHandler> CBHandler::Create(Callback callback) {
		CefRefPtr<CBHandler> ret = new CBHandler;
		ret->setCallback(std::move(callback));
		return ret;
	}

	bool CBHandler::Execute(CB_HANDLER_ARGS) {
		return handler(name, object, arguments, retval, exception);
	}

	void CBHandler::setCallback(Callback callback) {
		handler = std::move(callback);
	}
} // namespace Extendify::api::util
