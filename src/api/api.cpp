#include "api.hpp"

#include "log/log.hpp"
#include "settings.hpp"
#include "util/iter.hpp"
#include "util/string.hpp"

#include <cef_v8.h>
#include <utility>
#include <winerror.h>

namespace Extendify::api {
	log::Logger logger({"Extendify", "api"});

	void inject(const CefRefPtr<CefV8Context>& context) {
		logger.trace("Injecting API into context");
		CefRefPtr<CefV8Value> global = context->GetGlobal();
		CefRefPtr<CefV8Value> extendify = CefV8Value::CreateObject(nullptr, nullptr);

		extendify->SetValue("settings", settings::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		global->SetValue("extendify", extendify, V8_PROPERTY_ATTRIBUTE_NONE);
	}

	[[nodiscard]] V8Type getV8Type(const CefRefPtr<CefV8Value>& value) {
		E_ASSERT(value->IsValid() && "V8 value is not valid");

		if (value->IsUndefined()) {
			return V8Type::UNDEFINED;
		} else if (value->IsNull()) {
			return V8Type::NULL_TYPE;
		} else if (value->IsBool()) {
			return V8Type::BOOL;
		} else if (value->IsInt()) {
			return V8Type::INT;
		} else if (value->IsUInt()) {
			return V8Type::UINT;
		} else if (value->IsDouble()) {
			return V8Type::DOUBLE;
		} else if (value->IsDate()) {
			return V8Type::DATE;
		} else if (value->IsString()) {
			return V8Type::STRING;
		} else if (value->IsObject()) {
			return V8Type::OBJECT;
		} else if (value->IsArray()) {
			return V8Type::ARRAY;
		} else if (value->IsArrayBuffer()) {
			return V8Type::ARRAY_BUFFER;
		} else if (value->IsFunction()) {
			return V8Type::FUNCTION;
		} else if (value->IsPromise()) {
			return V8Type::PROMISE;
		}
		E_ASSERT(false && "Unknown V8 value type");
	};

	[[nodiscard]] std::string getTypeName(const CefRefPtr<CefV8Value>& value) {
		return getTypeName(getV8Type(value));
	};

	[[nodiscard]] constexpr std::string getTypeName(V8Type type) {
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

	APIUsage::APIUsage():
		usageString("unknown usage") {
	}

	[[nodiscard]] APIUsage::APIUsage(APIFunction func):
		func(std::move(func)),
		usageString(makeUsageString(this->func)) {
	}

	[[nodiscard]] constexpr std::string APIUsage::getUsage() const noexcept {
		return usageString;
	}

	void APIUsage::validateOrThrow(const CefV8ValueList& arguments) const noexcept(false) {
		if (auto err = validateArgs(arguments)) {
			throw std::runtime_error(*err);
		}
	};

	[[nodiscard]] std::optional<std::string>
	APIUsage::validateArgs(const CefV8ValueList& arguments) const noexcept {
		if (arguments.size() < func.expectedArgs.size()
			|| (func.expectedArgs.size() != arguments.size() && !func.allowTrailingArgs)) {
			return std::format("expected {} {} arguments, got {}",
							   func.allowTrailingArgs ? "at least" : "exactly",
							   func.expectedArgs.size(),
							   arguments.size());
		}
		for (auto i = 0; i < arguments.size(); i++) {
			const auto& arg = arguments[i];
			if (getV8Type(arguments[i]) != func.expectedArgs[i]) {
				goto err;
			}
		}
		return {};
err:
		return std::format(
			"Invalid usage. Expected {}. Got {}", getUsage(), makeActualUsageString(arguments));
	}

	[[nodiscard]] std::string
	APIUsage::makeActualUsageString(const CefV8ValueList& arguments) const noexcept {
		std::string ret;
		if (!func.path.empty()) {
			ret += func.path + "#";
		}
		if (func.name.empty()) {
			ret += "<func_name>";
		} else {
			ret += func.name;
		}
		ret += "(";
		ret += util::string::join(util::iter::map(arguments,
												  [](const CefRefPtr<CefV8Value>& arg) {
													  return getTypeName(getV8Type(arg));
												  }),
								  ", ");
		ret += ")";
		if (func.returnType) {
			ret += ": " + stringifyUnionType(*func.returnType);
		}
		return ret;
	}

	[[nodiscard]] constexpr std::string APIUsage::makeUsageString(const APIFunction& func) noexcept {
		if (func.name.empty()) {
			return "unknown usage";
		}
		std::string ret;
		if (!func.path.empty()) {
			ret += func.path + "#";
		}
		ret += func.name + "(";
		const auto args = typesToString(func.expectedArgs);
		ret += util::string::join(args, ", ");
		ret += ")";
		if (func.returnType) {
			ret += ": " + stringifyUnionType(*func.returnType);
		}
		return ret;
	}

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<V8Type>& types) noexcept {
		return util::iter::map(types, [](const V8Type& type) -> std::string {
			return getTypeName(type);
			;
		});
	}

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<uint64_t>& types) noexcept {
		#warning todo
	}

	[[nodiscard]] constexpr std::string APIUsage::stringifyUnionType(uint64_t type) noexcept {
		if (!type) {
			return {};
		}
		std::vector<V8Type> types;
		for (auto i = 1ULL; i < 2ULL << 62 && type; i <<= 1) {
			if (type & i) {
				type &= ~i;
				types.push_back(static_cast<V8Type>(i));
			}
		}
		return util::string::join(typesToString(types), " | ");
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
