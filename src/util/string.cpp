#include "string.hpp"

#include "log/log.hpp"

#include <cstdlib>
#include <regex>
#include <sstream>
#include <string>

namespace Extendify::util::string {

	[[nodiscard]] std::vector<std::string>
	split(const std::string& str, const std::string& delimiter,
		  const SplitOptions& opts) noexcept {
		if (!opts.limit) {
			return {};
		}
		if (delimiter.size() > str.size()) {
			return {str};
		}
		if (str.empty()) {
			return {str};
		}
		std::vector<std::string> res;
		int curCount = 0;
		if (delimiter.empty()) {
			for (const auto& c : str) {
				if (curCount++ == opts.limit) {
					break;
				}
				res.emplace_back(1, c);
			}
			return res;
		}
		auto lastPos = 0uz;
		auto nextPos = str.find(delimiter, lastPos);
		while (curCount++ != opts.limit) {
			if (nextPos == std::string::npos) {
				nextPos = str.size();
				// NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
				res.emplace_back(str.begin() + lastPos, str.begin() + nextPos);
			}
			// NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
			res.emplace_back(str.begin() + lastPos, str.begin() + nextPos);
			lastPos = nextPos + delimiter.size();
			nextPos = str.find(delimiter, lastPos);
			if (nextPos == str.size() - delimiter.size()) {
				res.emplace_back();
			}
		}
		return res;
	}

	[[nodiscard]] std::vector<std::string>
	split(const std::string& str, const std::basic_regex<char>& delimiter,
		  const SplitOptions& opts) {
		if (!opts.limit) {
			return {};
		}
		if (str.empty()) {
			return {str};
		}

		std::vector<std::string> res;

		std::smatch match;

		auto lastPos = str.begin();
		std::regex_search(lastPos, str.end(), match, delimiter, opts.flags);

		int curCount = 0;

		while (curCount++ != opts.limit) {
			if (match.empty()) {
				res.emplace_back(lastPos, str.end());
				break;
			}
			res.emplace_back(lastPos, str.begin() + match.position());
			lastPos = str.begin() + match.position() + match.length();
			std::regex_search(lastPos, str.end(), match, delimiter, opts.flags);
			if (match.position() + match.length() == str.size()) {
				res.emplace_back();
				break;
			}
		}
		return res;
	};

	constexpr static std::string whitespaceChars = " \t\n\r";

	void trimr(std::string& str) noexcept {
		if (str.empty()) {
			return;
		}
		// the trailing whitespace, if any, is not at the end of the string
		if (str.find_last_of(whitespaceChars) != str.size() - 1) {
			return;
		}
		const auto trimStart = str.find_last_not_of(whitespaceChars);
		if (trimStart == std::string::npos) {
			str.clear(); // all whitespace
			return;
		}
		// NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
		str.erase(str.begin() + trimStart + 1, str.end());
	}

	void triml(std::string& str) noexcept {
		if (str.empty()) {
			return;
		}
		// the leading whitespace, if any, is not at the start of the string
		if (str.find_first_of(whitespaceChars) != 0) {
			return;
		}
		const auto trimEnd = str.find_first_not_of(whitespaceChars);
		if (trimEnd == std::string::npos) {
			str.clear(); // all whitespace
			return;
		}
		// NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
		str.erase(str.begin(), str.begin() + trimEnd);
	}

	void replace(std::string& str, const std::basic_regex<char>& regex,
				 const std::string& replacement) {
		str = std::regex_replace(str, regex, replacement);
	}

	std::string wstringToString(std::wstring wstr) {
		if (wstr.empty()) {
			return "";
		}
		// reset state
		std::wctomb(nullptr, *wstr.begin());
		const auto maxSize = MB_CUR_MAX;
		auto buf = new char[MB_CUR_MAX];
		std::stringstream ret;
		for (const wchar_t wc : wstr) {
			// null chars can be in strings and not terminate them
			if (wc == L'\0') {
				ret << '\0';
				continue;
			}
			int count = std::wctomb(buf, wc);
			E_ASSERT(count <= maxSize && "wctomb returned more than max size");
			// https://en.wikipedia.org/wiki/Specials_(Unicode_block)
			if (count < 0) {
				ret << "\uFFFD"; // replacement character
				continue;
			}
			ret.write(buf, count);
		}
		delete[] buf;
		return ret.str();
	}

	// https://en.cppreference.com/w/cpp/string/multibyte/mbtowc
	std::wstring stringToWstring(std::string str) {
		if (str.empty()) {
			return L"";
		}
		// reset state
		std::mbtowc(nullptr, nullptr, 0);
		auto begin = str.begin();
		auto end = str.end();
		std::wstringstream ret;
		int count {};
		for (wchar_t wc;
			 (count = std::mbtowc(&wc, begin._Ptr, end - begin)) > 0;
			 begin += count)
			ret << wc;
		return ret.str();
	}
} // namespace Extendify::util::string
