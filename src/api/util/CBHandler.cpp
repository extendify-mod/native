#include "CBHandler.hpp"

namespace Extendify::api::util {
	CefRefPtr<CBHandler> CBHandler::Create(Callback h) {
		CefRefPtr<CBHandler> ret = new CBHandler;
		ret->setCallback(std::move(h));
		return ret;
	}

	bool CBHandler::Execute(CB_HANDLER_ARGS) {
		return handler(name, object, arguments, retval, exception);
	}

	void CBHandler::setCallback(Callback h) {
		handler = std::move(h);
	}
} // namespace Extendify::api::util
