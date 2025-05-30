#include "themes.hpp"

#include "api/util/CBHandler.hpp"
#include "fs/FilePicker.hpp"
#include "path/path.hpp"
#include "util/APIFunction.hpp"
#include "util/APIUsage.hpp"
#include "util/string.hpp"
#include "util/V8Type.hpp"

#include <cef_v8.h>
#include <internal/cef_ptr.h>
#include <regex>

static CefV8ValueList themeChangeListeners {};
static CefRefPtr<CefV8Context> themeChangeContext = nullptr;

using namespace Extendify::util;

namespace Extendify::api::themes {
	log::Logger logger({"Extendify", "api", "themes"});

	using util::APIFunction;
	using util::APIUsage;
	using util::CBHandler;
	using util::V8Type;

	namespace usage {
		APIUsage uploadTheme {APIFunction {
			.name = "uploadTheme",
			.description = "Prompts the user to upload a theme file",
			.path = "themes",
			.expectedArgs = {},
			.returnType = V8Type::PROMISE}};
		APIUsage deleteTheme {APIFunction {
			.name = "deleteTheme",
			.description =
				"Delete a theme by its file. The string should be the path to "
				"the theme "
				"file. Throws if the path is outside the theme directory.",
			.path = "themes",
			.expectedArgs = {V8Type::STRING},
			.returnType = V8Type::PROMISE,
		}};
		APIUsage addThemeChangeListener {APIFunction {
			.name = "addThemeChangeListener",
			.description = "Add a listener for theme changes (files added or "
						   "removed from the theme folder)",
			.path = "themes",
			.expectedArgs = {V8Type::FUNCTION},
			.returnType = V8Type::UNDEFINED,
		}};
		APIUsage getThemesDir {APIFunction {
			.name = "getThemesDir",
			.description = "Get the path to the themes directory",
			.path = "themes",
			.expectedArgs = {},
			.returnType = V8Type::STRING,
		}};
		/**
		 * @see UserTheme
		 *
		 * @returns array of UserTheme
		 */
		APIUsage getThemes {APIFunction {
			.name = "getThemes",
			.description = "Get the list of themes in the themes directory",
			.path = "themes",
			.expectedArgs = {},
			.returnType = V8Type::ARRAY,
		}};
		APIUsage getThemeData {APIFunction {
			.name = "getThemeData",
			.description = "Get the data for a theme by its filename",
			.path = "themes",
			.expectedArgs = {V8Type::STRING},
			.returnType = V8Type::OBJECT}};
	} // namespace usage

	static auto uploadThemeHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& name, CefRefPtr<CefV8Value> object,
		   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
		   CefString& exception) {
			try {
				usage::uploadTheme.validateOrThrow(arguments);
				auto picker = fs::FilePicker::pickOne();
				retval = picker->promise(
					CefV8Context::GetCurrentContext(),
					[](CefRefPtr<fs::FilePicker> picker,
					   std::vector<std::filesystem::path>
						   pickedPath,
					   std::optional<std::string>
						   error)
						-> std::expected<CefRefPtr<CefV8Value>, std::string> {
						if (error.has_value()) {
							return std::unexpected(std::move(*error));
						}
						if (pickedPath.empty()) {
							constexpr auto msg =
								"No file was picked for upload";
							logger.warn(msg);
							return std::unexpected(msg);
						}
						const auto& path = pickedPath[0];
						if (themeExists(path.filename().string())) {
							const auto msg =
								std::format("Theme with name {} already "
											"exists, not uploading",
											path.filename().string());
							logger.error(msg);
							return std::unexpected(msg);
						}
						std::filesystem::copy_file(
							path, path::getThemesDir(true) / path.filename());
						return CefV8Value::CreateUndefined();
					});
			} catch (std::exception& e) {
				const auto msg =
					std::format("Error uploading theme: {}", e.what());
				logger.error(msg);
				exception = msg;
			}
			return true;
		});

	static auto addChangeListenerHandler = CBHandler::Create(
		// NOLINTNEXTLINE(performance-unnecessary-value-param)
		[](const CefString& name, CefRefPtr<CefV8Value> object,
		   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
		   CefString& exception) {
			try {
				usage::addThemeChangeListener.validateOrThrow(arguments);
				const auto currentContext = CefV8Context::GetCurrentContext();
				if (!themeChangeContext) {
					themeChangeContext = currentContext;
				} else if (!themeChangeContext->IsSame(currentContext)) {
					logger.error("Saved theme change context is not the same "
								 "as the entered one while "
								 "adding a listener, invalidating all previous "
								 "listeners and using "
								 "the current context");
					themeChangeListeners.clear();
					themeChangeContext = currentContext;
				}
				themeChangeListeners.push_back(arguments[0]);
				logger.debug("Added theme change listener, total listeners: {}",
							 themeChangeListeners.size());
			} catch (std::exception& e) {
				const auto msg = std::format(
					"Error adding theme change listener: {}", e.what());
				logger.error(msg);
				exception = msg;
			}
			return true;
			;
		});

	[[nodiscard]] CefRefPtr<CefV8Value> makeApi() {
		CefRefPtr<CefV8Value> api = CefV8Value::CreateObject(nullptr, nullptr);

		CefRefPtr<CefV8Value> uploadThemeFunc =
			CefV8Value::CreateFunction("uploadTheme", uploadThemeHandler);
		api->SetValue(
			"uploadTheme", uploadThemeFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> addChangeListenerFunc =
			CefV8Value::CreateFunction("addChangeListener",
									   addChangeListenerHandler);
		api->SetValue("addChangeListener",
					  addChangeListenerFunc,
					  V8_PROPERTY_ATTRIBUTE_NONE);

		return api;
	}

	bool themeExists(const std::string& fileName) {
		return std::filesystem::exists(path::getThemesDir() / fileName);
	};

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
			logger.warn("Attempting to set an empty value for field: {}",
						field);
		}

		if (field == fileName) {
			logger.warn("Attempting to set the fileName of a UserTheme, this "
						"is not allowed");
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

	UserTheme makeHeader(const std::string& fileName,
						 const UserTheme& previousThemeData) {
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

	UserTheme getThemeInfo(const std::string& css,
						   const std::string& fileName) {
		UserTheme theme;
		if (css.empty()) {
			logger.warn("making theme info for empty css with filename: {}",
						fileName);
			return std::move(makeHeader(fileName, theme));
		}
		std::string block;
		{
			const auto first = string::split(css, "/**", {.limit = 2});
			if (first.size() < 2) {
				return std::move(makeHeader(fileName, theme));
			}
			const auto end = string::split(first[1], "*/", {.limit = 1});
			if (!end.size()) {
				return std::move(makeHeader(fileName, theme));
			}
			block = end[0];
		}
		const auto lines = string::split(block, lineRegex);
		std::string field;
		std::string accum;
		for (const auto& line : lines) {
			if (line.empty()) {
				continue;
			}
			if (line[0] == '@' && line[1] != ' ') {
				// start new field
				string::trim(accum);
				theme.set(field, std::move(accum));
				const auto l = line.find(' ');
				assert(l != std::string::npos && "TODO: handle this");
				field = line.substr(1, l);
				accum = line.substr(l + 1);
			} else {
				std::string l = line;
				if (const auto pos = line.find("\\n");
					pos != std::string::npos) {
					l = l.replace(pos, 2, "\n");
				}
				string::replace(l, escapedAtRegex, "@");
			}
		}
		string::trim(accum);
		theme.set(field, std::move(accum));
		return std::move(makeHeader(fileName, theme));
	}

} // namespace Extendify::api::themes
