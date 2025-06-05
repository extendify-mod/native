#pragma once
#include "APIFunction.hpp"
#include "V8Type.hpp"

#include <cef_v8.h>

namespace Extendify::api::util {

	class APIUsage {
	  public:
		[[nodiscard]] explicit APIUsage(APIFunction func);

		[[nodiscard]] constexpr std::string getUsage() const noexcept;

		void validateOrThrow(const CefV8ValueList& arguments) const
			noexcept(false);

		[[nodiscard]] std::optional<std::string>
		validateArgs(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] std::string
		makeActualUsageString(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] static std::string
		makeUsageString(const APIFunction& func) noexcept;

		/**
		 * @brief converts a vector of V8Type to a vector of strings for each
		 * type
		 *
		 * does not accept union types
		 *
		 * @param types the types to convert
		 * @return std::vector<std::string>
		 */
		[[nodiscard]] constexpr static std::vector<std::string>
		typesToString(const std::vector<V8Type>& types) noexcept;

		[[nodiscard]] constexpr static std::vector<std::string>
		typesToString(const std::vector<uint16_t>& types) noexcept;

		[[nodiscard]] constexpr static std::string
		stringifyUnionType(uint16_t type) noexcept;

	  private:
		APIFunction func;
	};
} // namespace Extendify::api::util
