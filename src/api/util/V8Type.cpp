#include "V8Type.hpp"

#include "log/log.hpp"

#include <cef_v8.h>
#include <internal/cef_ptr.h>
#include <string>


namespace Extendify::api::util {

	[[nodiscard]] V8Type getV8Type(const CefRefPtr<CefV8Value>& value) {
		E_ASSERT(value->IsValid() && "V8 value is not valid");

		if (value->IsUndefined()) {
			return V8Type::UNDEFINED;
		}
		if (value->IsNull()) {
			return V8Type::NULL_TYPE;
		}
		if (value->IsBool()) {
			return V8Type::BOOL;
		}
		if (value->IsInt()) {
			return V8Type::INT;
		}
		if (value->IsUInt()) {
			return V8Type::UINT;
		}
		if (value->IsDouble()) {
			return V8Type::DOUBLE;
		}
		if (value->IsDate()) {
			return V8Type::DATE;
		}
		if (value->IsString()) {
			return V8Type::STRING;
		}
		if (value->IsArray()) {
			return V8Type::ARRAY;
		}
		if (value->IsArrayBuffer()) {
			return V8Type::ARRAY_BUFFER;
		}
		if (value->IsFunction()) {
			return V8Type::FUNCTION;
		}
		if (value->IsPromise()) {
			return V8Type::PROMISE;
		}
		if (value->IsObject()) {
			return V8Type::OBJECT;
		}
		E_ASSERT(false && "Unknown V8 value type");
	};

	[[nodiscard]] std::string getTypeName(const CefRefPtr<CefV8Value>& value) {
		return getTypeName(getV8Type(value));
	};

	[[nodiscard]] std::string getTypeName(V8Type type) {
		switch (type) {
			case V8Type::UNDEFINED:
				return "undefined";
			case V8Type::NULL_TYPE:
				return "null";
			case V8Type::BOOL:
				return "bool";
			case V8Type::INT:
				return "int";
			case V8Type::UINT:
				return "uint";
			case V8Type::DOUBLE:
				return "double";
			case V8Type::DATE:
				return "date";
			case V8Type::STRING:
				return "string";
			case V8Type::OBJECT:
				return "object";
			case V8Type::ARRAY:
				return "array";
			case V8Type::ARRAY_BUFFER:
				return "array_buffer";
			case V8Type::FUNCTION:
				return "function";
			case V8Type::PROMISE:
				return "promise";
		}
		E_ASSERT(false && "Unknown v8 type");
	}
} // namespace Extendify::api::util
