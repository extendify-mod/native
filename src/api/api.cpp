#include "api.hpp"

namespace Extendify::api {
	log::Logger logger({"Extendify", "api"});
}

using namespace Extendify;
using namespace api;

void api::inject(const CefRefPtr<CefV8Context>& context) {
	logger.trace("Injecting API into context");
	CefRefPtr<CefV8Value> global = context->GetGlobal();
	CefRefPtr<CefV8Value> extendify = CefV8Value::CreateObject(nullptr, nullptr);
	global->SetValue("extendify", extendify, V8_PROPERTY_ATTRIBUTE_NONE);
	extendify->SetValue("api", CefV8Value::CreateString("api"), V8_PROPERTY_ATTRIBUTE_NONE);
}
