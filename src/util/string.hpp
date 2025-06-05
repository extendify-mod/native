#pragma once

#include <ranges>
#include <regex>
#include <string>
#include <vector>

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

	struct SplitOptions {
		int limit = -1;
		std::regex_constants::match_flag_type flags =
			std::regex_constants::match_default;
	};

	[[nodiscard]] std::vector<std::string>
	split(const std::string& str, const std::string& delimiter,
		  const SplitOptions& opts = {}) noexcept;

	[[nodiscard]] std::vector<std::string>
	split(const std::string& str, const std::basic_regex<char>& delimiter,
		  const SplitOptions& opts = {});

	void trimr(std::string& str) noexcept;
	void triml(std::string& str) noexcept;

	constexpr void trim(std::string& str) noexcept {
		triml(str);
		trimr(str);
	};

	void replace(std::string& str, const std::basic_regex<char>& regex,
				 const std::string& replacement);
	[[nodiscard]] std::string wstringToString(std::wstring wstr);
	[[nodiscard]] std::wstring stringToWstring(std::string str);
} // namespace Extendify::util::string
