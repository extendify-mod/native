#pragma once

#include <iterator>
#include <string>

namespace Extendify::util::string {
	template<std::indirectly_readable T>
	std::string join(T strings, const std::string& delimiter) {
		std::string ret;

		for (const auto& str : strings) {
			ret += str;
			ret += delimiter;
		}

		ret.pop_back();
		return ret;
	};
} // namespace Extendify::util::string
