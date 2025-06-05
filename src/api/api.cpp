#include "api.hpp"

#include "log/Logger.hpp"
#include "quickCss.hpp"
#include "settings.hpp"
#include "themes.hpp"

#include <cef_v8.h>
#include <internal/cef_ptr.h>
#include <internal/cef_types.h>

namespace Extendify::api {
	log::Logger logger({"Extendify", "api"});

	void inject(const CefRefPtr<CefV8Context>& context) {
		logger.info("Injecting ExtendifyNative API into V8 context");
		CefRefPtr<CefV8Value> global = context->GetGlobal();
		CefRefPtr<CefV8Value> extendify =
			CefV8Value::CreateObject(nullptr, nullptr);

		extendify->SetValue(
			"settings", settings::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		extendify->SetValue(
			"quickCss", quickCss::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		extendify->SetValue(
			"themes", themes::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		global->SetValue(
			"ExtendifyNative", extendify, V8_PROPERTY_ATTRIBUTE_NONE);
		global->SetValue("EXTENDIFY_NATIVE_AVAILABLE",
						 CefV8Value::CreateBool(true),
						 V8_PROPERTY_ATTRIBUTE_NONE);
	}

} // namespace Extendify::api
