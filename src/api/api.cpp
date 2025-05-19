#include "api.hpp"

#include "settings.hpp"

namespace Extendify::api {
	log::Logger logger({"Extendify", "api"});

} // namespace Extendify::api

using namespace Extendify;
using namespace api;

void api::inject(const CefRefPtr<CefV8Context>& context) {
	logger.trace("Injecting API into context");
	CefRefPtr<CefV8Value> global = context->GetGlobal();
	CefRefPtr<CefV8Value> extendify = CefV8Value::CreateObject(nullptr, nullptr);

	extendify->SetValue("settings", settings::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

	global->SetValue("extendify", extendify, V8_PROPERTY_ATTRIBUTE_NONE);
}

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
