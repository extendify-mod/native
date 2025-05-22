#include "api.hpp"

#include "settings.hpp"

namespace Extendify::api {
	log::Logger logger({"Extendify", "api"});

	std::string getTypeName(const CefRefPtr<CefV8Value>& value) {
		if (!value->IsValid()) {
			return "INVALID VALUE";
		}

		if (value->IsUndefined()) {
			return "undefined";
		} else if (value->IsNull()) {
			return "null";
		} else if (value->IsBool()) {
			return "boolean";
		} else if (value->IsInt()) {
			return "integer";
		} else if (value->IsUInt()) {
			return "unsigned integer";
		} else if (value->IsDouble()) {
			return "double";
		} else if (value->IsDate()) {
			return "date";
		} else if (value->IsString()) {
			return "string";
		} else if (value->IsObject()) {
			return "object";
		} else if (value->IsArray()) {
			return "array";
		} else if (value->IsArrayBuffer()) {
			return "array buffer";
		} else if (value->IsFunction()) {
			return "function";
		} else if (value->IsPromise()) {
			return "promise";
		} else {
			logger.error("Unknown V8 value type");
			return "UNKNOWN TYPE";
		}
	};

	void inject(const CefRefPtr<CefV8Context>& context) {
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
} // namespace Extendify::api
