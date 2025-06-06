#include "string.hpp"

#include <cstdlib>
#include <regex>
#include <string>
#include <vector>

#ifdef _WIN32
#include "log/log.hpp"

#include <memory>
#include <sstream>
#endif

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
			for (const auto& curChar : str) {
				if (curCount++ == opts.limit) {
					break;
				}
				res.emplace_back(1, curChar);
			}
			return res;
		}
		auto lastPos = 0UZ;
		auto nextPos = str.find(delimiter, lastPos);
		while (curCount++ != opts.limit) {
			if (nextPos == std::string::npos) {
				nextPos = str.size();
				res.emplace_back(str.begin() + lastPos, str.begin() + nextPos);
				break;
			}
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
		const auto doSearch = [&] {
			std::regex_search(lastPos, str.end(), match, delimiter, opts.flags);
		};
		doSearch();

		int curCount = 0;
		if (match.position() == 0) {
			lastPos = str.begin() + match.length();
			doSearch();
		} else if (match.empty()) {
			return {str};
		}
		while (curCount++ != opts.limit) {
			doSearch();
			res.emplace_back(
				lastPos,
				(match.empty() ? str.end() : lastPos + match.position()));
			lastPos += match.position() + match.length();
			if (lastPos > str.end()) {
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

#ifdef _WIN32
	std::string wstringToString(std::wstring wstr) {
		if (wstr.empty()) {
			return "";
		}
		// reset state
		std::wctomb(nullptr, *wstr.begin());
		const auto maxSize = MB_CUR_MAX;
		std::unique_ptr<char[]> buf(new char[maxSize]);
		std::stringstream ret;
		for (const wchar_t curChar : wstr) {
			// null chars can be in strings and not terminate them
			if (curChar == L'\0') {
				ret << '\0';
				continue;
			}
			int count = std::wctomb(buf.get(), curChar);
			E_ASSERT(count <= maxSize && "wctomb returned more than max size");
			// https://en.wikipedia.org/wiki/Specials_(Unicode_block)
			if (count < 0) {
				ret << "\uFFFD"; // replacement character
				continue;
			}
			ret.write(buf.get(), count);
		}
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
		for (wchar_t wc {};
			 (count = std::mbtowc(&wc, begin._Ptr, end - begin)) > 0;
			 begin += count) {
			ret << wc;
		}
		return ret.str();
	}
#endif
} // namespace Extendify::util::string
