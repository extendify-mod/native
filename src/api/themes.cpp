#include "themes.hpp"

#include "api.hpp"
#include "util/string.hpp"

#include <regex>

namespace Extendify::api::themes {
	log::Logger logger({"Extendify", "api", "themes"});
	
	namespace usage {
		APIUsage uploadTheme {APIFunction {
			.name = "uploadTheme",
			.description = "Prompts the user to upload a theme file",
			.path = "themes",

		}};
	}

	[[nodiscard]] CefRefPtr<CefV8Value> makeApi() {

	}
	/*!
	 * BetterDiscord addon meta parser
	 * Copyright 2023 BetterDiscord contributors
	 * Copyright 2023 Vendicated and Vencord contributors
	 *
	 * Licensed under the Apache License, Version 2.0 (the "License");
	 * you may not use this file except in compliance with the License.
	 * You may obtain a copy of the License at
	 *
	 *   http://www.apache.org/licenses/LICENSE-2.0
	 *
	 * Unless required by applicable law or agreed to in writing, software
	 * distributed under the License is distributed on an "AS IS" BASIS,
	 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	 * See the License for the specific language governing permissions and
	 * limitations under the License.
	 */
	void UserTheme::set(const std::string& field, std::string value) {
		if (field.empty()) {
			return;
		}

		if (value.empty()) {
			logger.warn("Attempting to set an empty value for field: {}", field);
		}

		if (field == fileName) {
			logger.warn("Attempting to set the fileName of a UserTheme, this is not allowed");
		} else if (field == "name") {
			name = std::move(value);
		} else if (field == "author") {
			author = std::move(value);
		} else if (field == "description") {
			description = std::move(value);
		} else if (field == "version") {
			version = std::move(value);
		} else if (field == "license") {
			license = std::move(value);
		} else if (field == "source") {
			source = std::move(value);
		} else if (field == "website") {
			website = std::move(value);
		} else if (field == "invite") {
			invite = std::move(value);
		} else {
			logger.warn("Unknown field in UserTheme: {}", field);
		}
	};

	// from ECMAScript regex
	// /[^\S\r\n]*?\r?(?:\r\n|\n)[^\S\r\n]*?\*[^\S\r\n]?/;
	const static std::basic_regex<char> lineRegex {
		R"([^\S\r\n]*?\r?(?:\r\n|\n)[^\S\r\n]*?\*[^\S\r\n]?)"};
	const static std::basic_regex<char> escapedAtRegex {R"(^\\@)"};

	UserTheme makeHeader(const std::string& fileName, const UserTheme& previousThemeData) {
		UserTheme theme;
		theme.fileName = fileName;
		if (!previousThemeData.name.empty()) {
			theme.name = previousThemeData.name;
		} else {
			if (fileName.ends_with(".css")) {
				theme.name = fileName.substr(0, fileName.size() - 4);
			} else {
				theme.name = fileName;
			}
		}

		theme.author = previousThemeData.author;
		theme.description = previousThemeData.description;

		theme.version = previousThemeData.version;
		theme.license = previousThemeData.license;
		theme.source = previousThemeData.source;
		theme.website = previousThemeData.website;
		theme.invite = previousThemeData.invite;
		return std::move(theme);
	}

	UserTheme getThemeInfo(const std::string& css, const std::string& fileName) {
		UserTheme theme;
		if (css.empty()) {
			logger.warn("making theme info for empty css with filename: {}", fileName);
			return std::move(makeHeader(fileName, theme));
		}
		std::string block;
		{
			const auto first = util::string::split(css, "/**", {.limit = 2});
			if (first.size() < 2) {
				return std::move(makeHeader(fileName, theme));
			}
			const auto end = util::string::split(first[1], "*/", {.limit = 1});
			if (!end.size()) {
				return std::move(makeHeader(fileName, theme));
			}
			block = end[0];
		}
		const auto lines = util::string::split(block, lineRegex);
		std::string field;
		std::string accum;
		for (const auto& line : lines) {
			if (line.empty()) {
				continue;
			}
			if (line[0] == '@' && line[1] != ' ') {
				// start new field
				util::string::trim(accum);
				theme.set(field, std::move(accum));
				const auto l = line.find(' ');
				assert(l != std::string::npos && "TODO: handle this");
				field = line.substr(1, l);
				accum = line.substr(l + 1);
			} else {
				std::string l = line;
				if (const auto pos = line.find("\\n"); pos != std::string::npos) {
					l = l.replace(pos, 2, "\n");
				}
				util::string::replace(l, escapedAtRegex, "@");
			}
		}
		util::string::trim(accum);
		theme.set(field, std::move(accum));
		return std::move(makeHeader(fileName, theme));
	}

} // namespace Extendify::api::themes
