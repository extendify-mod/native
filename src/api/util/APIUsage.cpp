#include "APIUsage.hpp"

#include "APIFunction.hpp"
#include "util/iter.hpp"
#include "util/string.hpp"
#include "V8Type.hpp"

#include <cef_v8.h>
#include <cstdint>
#include <format>
#include <internal/cef_ptr.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


namespace Extendify::api::util {
	[[nodiscard]] APIUsage::APIUsage(APIFunction func):
		func(std::move(func)) {
	}

	[[nodiscard]] constexpr std::string APIUsage::getUsage() const noexcept {
		if (func.name.empty()) {
			return "unknown usage";
		}
		std::ostringstream ret;
		if (!func.path.empty()) {
			ret << func.path << "#";
		}
		ret << func.name << "(";
		const auto args = typesToString(func.expectedArgs);
		ret << Extendify::util::string::join(args, ", ") << ")";
		if (func.returnType) {
			ret << ": " << stringifyUnionType(*func.returnType);
		}
		return ret.str();
	}

	void APIUsage::validateOrThrow(const CefV8ValueList& arguments) const
		noexcept(false) {
		if (auto err = validateArgs(arguments)) {
			throw std::runtime_error(*err);
		}
	};

	[[nodiscard]] std::optional<std::string>
	APIUsage::validateArgs(const CefV8ValueList& arguments) const noexcept {
		if (arguments.size() < func.expectedArgs.size()
			|| (func.expectedArgs.size() != arguments.size()
				&& !func.allowTrailingArgs)) {
			return std::format("expected {} {} arguments, got {}",
							   func.allowTrailingArgs ? "at least" : "exactly",
							   func.expectedArgs.size(),
							   arguments.size());
		}
		for (auto i = 0; i < arguments.size(); i++) {
			if (getV8Type(arguments[i]) != func.expectedArgs[i]) {
				return std::format("Invalid usage. Expected {}. Got {}",
								   getUsage(),
								   makeActualUsageString(arguments));
			}
		}
		return {};
	}

	[[nodiscard]] std::string APIUsage::makeActualUsageString(
		const CefV8ValueList& arguments) const noexcept {
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
		ret += Extendify::util::string::join(
			Extendify::util::iter::map(arguments,
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

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<V8Type>& types) noexcept {
		return Extendify::util::iter::map(
			types, [](const V8Type& type) -> std::string {
				return getTypeName(type);
				;
			});
	}

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<uint16_t>& types) noexcept {
		return Extendify::util::iter::map(types,
										  [](uint64_t type) -> std::string {
											  return stringifyUnionType(type);
										  });
	}

	[[nodiscard]] constexpr std::string
	APIUsage::stringifyUnionType(uint16_t type) noexcept {
		if (!type) {
			return {};
		}
		std::vector<V8Type> types;
		for (auto i = 1ULL; i < 1ULL << 63 && type; i <<= 1) {
			if (type & i) {
				type &= ~i;
				types.push_back(static_cast<V8Type>(i));
			}
		}
		return Extendify::util::string::join(typesToString(types), " | ");
	}

} // namespace Extendify::api::util
