#pragma once

#include <iterator>
#include <string>
#include <algorithm>

namespace Extendify::util::string {
	template <std::indirectly_readable T>
	std::string join(T begin, T end, const std::string& delimiter) {
		std::string ret;

		for (; begin != end; ++begin) {
			ret += *begin;
			ret += delimiter;
		}

		ret.pop_back();
		return ret;
	};
} // namespace Extendify::util::string
