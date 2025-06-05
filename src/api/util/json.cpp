#include "json.hpp"

#include "api/api.hpp"

#include <cef_v8.h>
#include <format>
#include <internal/cef_ptr.h>
#include <internal/cef_string.h>
#include <stdexcept>
#include <string>

namespace Extendify::api::util::json {
	namespace {
		CefRefPtr<CefV8Value> getJsonMethod(const std::string& method) {
			CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
			CefRefPtr<CefV8Value> window = context->GetGlobal();

			if (!window->HasValue("JSON")) {
				logger.error("window.JSON does not exist");
				throw std::runtime_error("window.JSON does not exist");
			}
			auto JSON = window->GetValue("JSON");
			if (!JSON->HasValue(method)) {
				logger.error("window.JSON.{} does not exist", method);
				throw std::runtime_error(
					std::format("window.JSON.{} does not exist", method));
			}
			CefRefPtr<CefV8Value> toRet = JSON->GetValue(method);
			if (!toRet->IsFunction()) {
				logger.error("window.JSON.{} is not a function", method);
				throw std::runtime_error(
					std::format("window.JSON.{} is not a function", method));
			}
			return toRet;
		}
	} // namespace

	CefRefPtr<CefV8Value> parse(const CefString& json) {
		CefRefPtr<CefV8Value> jsonString = CefV8Value::CreateString(json);
		return parse(jsonString);
	};

	CefRefPtr<CefV8Value> parse(const CefRefPtr<CefV8Value>& json) {
		auto jsonParse = getJsonMethod("parse");
		auto ret = jsonParse->ExecuteFunction(nullptr, {json});

		if (jsonParse->HasException()) {
			const auto msg = jsonParse->GetException()->GetMessage();
			logger.error("JSON.parse exception: {}", msg.ToString());
			throw std::runtime_error(msg);
		}

		return ret;
	};

	std::string stringify(const CefRefPtr<CefV8Value>& obj) {
		auto stringify = getJsonMethod("stringify");
		auto ret = stringify->ExecuteFunction(nullptr, {obj});

		if (stringify->HasException()) {
			const auto msg = stringify->GetException()->GetMessage();
			logger.error("JSON.stringify exception: {}", msg.ToString());
			throw std::runtime_error(msg);
		}
		return ret->GetStringValue();
	};
} // namespace Extendify::api::util::json
