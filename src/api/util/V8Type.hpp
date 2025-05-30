#pragma once
#include <cef_v8.h>
#include <string>

namespace Extendify::api::util {
	enum V8Type {
		UNDEFINED = 1,
		NULL_TYPE = 2 << 0,
		BOOL = 2 << 1,
		INT = 2 << 2,
		UINT = 2 << 3,
		DOUBLE = 2 << 4,
		DATE = 2 << 5,
		STRING = 2 << 6,
		OBJECT = 2 << 7,
		ARRAY = 2 << 8,
		ARRAY_BUFFER = 2 << 9,
		FUNCTION = 2 << 10,
		PROMISE = 2 << 11,
	};

	/**
	 * @brief gets the type for a v8 value
	 *
	 * @param value a v8 value
	 * @invariant @param value is valid
	 * @return V8Type
	 */
	[[nodiscard]] V8Type getV8Type(const CefRefPtr<CefV8Value>& value);

	/**
	 * @brief Get the type of value
	 *
	 * @param value
	 * @invariant @param value is valid
	 * @return const std::string&
	 */
	[[nodiscard]] std::string getTypeName(const CefRefPtr<CefV8Value>& value);
	[[nodiscard]] std::string getTypeName(V8Type type);
} // namespace Extendify::api::util
