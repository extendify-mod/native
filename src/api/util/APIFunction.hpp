#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Extendify::api::util {
	struct APIFunction {
		std::string name;
		std::string description;
		std::string path;
		bool allowTrailingArgs = false;
		/**
		 * @brief array of parameter types, bitwise OR'd together
		 * @see V8Type
		 */
		const std::vector<uint64_t> expectedArgs;
		/**
		 * @brief  return type, bitwise OR'd together
		 * @see V8Type
		 */
		std::optional<uint64_t> returnType;
	};
} // namespace Extendify::api::util
