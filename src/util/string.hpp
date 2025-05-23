#pragma once

#include <ranges>
#include <iterator>
#include <string>


namespace Extendify::util::string {
	template<typename T>
	constexpr std::string join(T iter, const std::string& delimiter) {
		std::string ret;
		auto begin = std::ranges::begin(iter);
		auto end = std::ranges::end(iter);
		for (; begin != end; ++begin) {
			ret += *begin;
			ret += delimiter;
		}
		// NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
		ret.erase(ret.end() - delimiter.size(), ret.end());
		return ret;
	};
} // namespace Extendify::util::string
