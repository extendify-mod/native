#include "themes.hpp"

#include "api/themes.hpp"
#include "api/util/CBHandler.hpp"
#include "fs/FilePicker.hpp"
#include "fs/fs.hpp"
#include "fs/Watcher.hpp"
#include "log/Logger.hpp"
#include "main.hpp"
#include "path/path.hpp"
#include "util/APIFunction.hpp"
#include "util/APIUsage.hpp"
#include "util/MessageBox.hpp"
#include "util/string.hpp"
#include "util/TaskCBHandler.hpp"
#include "util/V8Type.hpp"

#include <algorithm>
#include <cassert>
#include <cef_task.h>
#include <cef_v8.h>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <internal/cef_ptr.h>
#include <internal/cef_string.h>
#include <internal/cef_types.h>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace Extendify::api::themes {
	namespace {
		CefV8ValueList themeChangeListeners {};
		CefRefPtr<CefV8Context> themeChangeContext = nullptr;
		ThemeCache themeCache;
	} // namespace

	log::Logger logger({"Extendify", "api", "themes"});

	using util::APIFunction;
	using util::APIUsage;
	using util::CBHandler;
	using util::V8Type;
	using namespace Extendify::util;

	namespace usage {

		const APIUsage uploadTheme {APIFunction {
			.name = "uploadTheme",
			.description = "Prompts the user to upload a theme file",
			.path = "themes",
			.expectedArgs = {},
			.returnType = V8Type::PROMISE}};
		const APIUsage deleteTheme {APIFunction {
			.name = "deleteTheme",
			.description =
				"Delete a theme by its file. The string should be the path to "
				"the theme "
				"file. Throws if the path is outside the theme directory.",
			.path = "themes",
			.expectedArgs = {V8Type::STRING},
			.returnType = V8Type::PROMISE,
		}};
		const APIUsage addChangeListener {APIFunction {
			.name = "addThemeChangeListener",
			.description = "Add a listener for theme changes (files added or "
						   "removed from the theme folder)",
			.path = "themes",
			.expectedArgs = {V8Type::FUNCTION},
			.returnType = V8Type::UNDEFINED,
		}};
		const APIUsage getThemesDir {APIFunction {
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
		const APIUsage getThemes {APIFunction {
			.name = "getThemes",
			.description = "Get the list of themes in the themes directory",
			.path = "themes",
			.expectedArgs = {},
			.returnType = V8Type::ARRAY,
		}};
		const APIUsage getThemeData {APIFunction {
			.name = "getThemeData",
			.description = "Get the data for a theme by its filename",
			.path = "themes",
			.expectedArgs = {V8Type::STRING},
			.returnType = V8Type::OBJECT | V8Type::UNDEFINED}};
	} // namespace usage

	namespace {
		auto uploadThemeHandler = CBHandler::Create(
			// NOLINTNEXTLINE(performance-unnecessary-value-param)
			[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
			   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
			   CefString& exception) {
				try {
					usage::uploadTheme.validateOrThrow(arguments);
					auto picker = fs::FilePicker::Create(
						{.mode = fs::FilePicker::DialogType::OPEN,
						 .title = "Select a theme file to upload",
						 .filters = {{
							 .displayName = "All Files",
							 .patterns = {"*.*"},
						 }},
						 .defaultFilter =
							 {
								 .displayName = "CSS Files",
								 .patterns = {"*.css", "*.theme.css"},
							 },
						 .stateId = &ids::THEMES});
					retval = picker->promise(
						CefV8Context::GetCurrentContext(),
						// NOLINTNEXTLINE(performance-unnecessary-value-param)
						[](std::shared_ptr<fs::FilePicker> /*picker*/,
						   std::vector<std::filesystem::path>
							   pickedPath,
						   std::optional<std::string>
							   error) -> std::expected<CefRefPtr<CefV8Value>,
													   std::string> {
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
							std::filesystem::copy_file(path,
													   path::getThemesDir(true)
														   / path.filename());
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

		auto deleteThemeHandler = CBHandler::Create(
			// NOLINTNEXTLINE(performance-unnecessary-value-param)
			[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
			   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
			   CefString& exception) {
				try {
					usage::deleteTheme.validateOrThrow(arguments);
					const std::filesystem::path themePath {
						arguments[0]->GetStringValue()};
					if (!themePath.is_absolute()) {
						throw std::runtime_error("Theme path must be absolute");
					}
					if (themePath.lexically_relative(path::getThemesDir())
							.lexically_normal()
							.string()
							.starts_with("..")) {
						throw std::runtime_error(
							"Theme path must be inside the themes directory");
					}
					auto msgbox = Extendify::util::MsgBox::Create(
						"Delete Theme",
						std::format(
							"Are you sure you want to delete the file at: {}",
							themePath.string()),
						MsgBox::Type::YES_NO);
					retval = msgbox->promise(
						CefV8Context::GetCurrentContext(),
						[themePath](
							// NOLINTNEXTLINE(performance-unnecessary-value-param)
							std::shared_ptr<Extendify::util::MsgBox> /*msgbox*/,
							Extendify::util::MsgBox::Result res)
							-> std::expected<CefRefPtr<CefV8Value>,
											 std::string> {
							if (res == Extendify::util::MsgBox::Result::OK) {
								std::error_code errCode;
								std::filesystem::remove(themePath, errCode);
								if (errCode) {
									const auto msg = std::format(
										"Error deleting theme file: {}",
										errCode.message());
									logger.error(msg);
									return std::unexpected(msg);
								}
								return CefV8Value::CreateBool(true);
							}
							if (res
								== Extendify::util::MsgBox::Result::CANCEL) {
								return CefV8Value::CreateBool(false);
							}
							return std::unexpected(
								"MessageBox returned an unexpected result");
						});
				} catch (std::exception& e) {
					const auto msg =
						std::format("Error deleting theme: {}", e.what());
					logger.error(msg);
					exception = msg;
				}
				return true;
			});

		auto addChangeListenerHandler = CBHandler::Create(
			// NOLINTNEXTLINE(performance-unnecessary-value-param)
			[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
			   const CefV8ValueList& arguments,
			   CefRefPtr<CefV8Value>& /*retval*/, CefString& exception) {
				try {
					usage::addChangeListener.validateOrThrow(arguments);
					const auto currentContext =
						CefV8Context::GetCurrentContext();
					if (!themeChangeContext) {
						themeChangeContext = currentContext;
					} else if (!themeChangeContext->IsSame(currentContext)) {
						logger.error(
							"Saved theme change context is not the same "
							"as the entered one while "
							"adding a listener, invalidating all previous "
							"listeners and using "
							"the current context");
						themeChangeListeners.clear();
						themeChangeContext = currentContext;
					}
					themeChangeListeners.push_back(arguments[0]);
					logger.debug(
						"Added theme change listener, total listeners: {}",
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

		auto getThemesDirHandler = CBHandler::Create(
			// NOLINTNEXTLINE(performance-unnecessary-value-param)
			[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
			   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
			   CefString& exception) {
				try {
					usage::getThemesDir.validateOrThrow(arguments);
					const auto themesDir = path::getThemesDir();
					retval = CefV8Value::CreateString(themesDir.string());
				} catch (std::exception& e) {
					const auto msg =
						std::format("Error getting themes dir: {}", e.what());
					logger.error(msg);
					exception = msg;
				}
				return true;
			});

		auto getThemesHandler = CBHandler::Create(
			// NOLINTNEXTLINE(performance-unnecessary-value-param)
			[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
			   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
			   CefString& exception) {
				try {
					usage::getThemes.validateOrThrow(arguments);
					const auto themes = themeCache.getThemes();
					const auto arr =
						CefV8Value::CreateArray((int)themes.size());
					for (auto i = 0; i < themes.size(); i++) {
						arr->SetValue(i, themes[i]->toJSON());
					}
					retval = arr;
				} catch (std::exception& e) {
					const auto msg =
						std::format("Error getting themes: {}", e.what());
					logger.error(msg);
					exception = msg;
				}
				return true;
				;
			});

		auto getThemeDataHandler = CBHandler::Create(
			// NOLINTNEXTLINE(performance-unnecessary-value-param)
			[](const CefString& /*name*/, CefRefPtr<CefV8Value> /*object*/,
			   const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
			   CefString& exception) {
				try {
					usage::getThemeData.validateOrThrow(arguments);
					const auto theme = themeCache.getFromFileName(
						arguments[0]->GetStringValue());
					if (theme) {
						retval = (*theme)->toJSON();
					} else {
						retval = CefV8Value::CreateUndefined();
					}
				} catch (std::exception& e) {
					const auto msg =
						std::format("Error getting theme data: {}", e.what());
					logger.error(msg);
					exception = msg;
				}
				return true;
			});
	} // namespace

	[[nodiscard]] CefRefPtr<CefV8Value> makeApi() {
		[[maybe_unused]] const static int _ = []() {
			themeCache.initThemes();
			auto ret = fs::Watcher::get()->addDir(
				path::getThemesDir(true),
				[](std::unique_ptr<fs::Watcher::Event> event) -> void {
					themeCache.dispatchThemeUpdate(event->path);
				});
			if (!ret) {
				logger.error("Failed to add themes directory to watcher: {}",
							 ret.error());
			}
			return 0;
		}();
		CefRefPtr<CefV8Value> api = CefV8Value::CreateObject(nullptr, nullptr);

		CefRefPtr<CefV8Value> uploadThemeFunc =
			CefV8Value::CreateFunction("uploadTheme", uploadThemeHandler);
		api->SetValue(
			"uploadTheme", uploadThemeFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> deleteThemeFunc =
			CefV8Value::CreateFunction("deleteTheme", deleteThemeHandler);
		api->SetValue(
			"deleteTheme", deleteThemeFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> addChangeListenerFunc =
			CefV8Value::CreateFunction("addChangeListener",
									   addChangeListenerHandler);
		api->SetValue("addChangeListener",
					  addChangeListenerFunc,
					  V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> getThemesDirFunc =
			CefV8Value::CreateFunction("getThemesDir", getThemesDirHandler);
		api->SetValue(
			"getThemesDir", getThemesDirFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> getThemesFunc =
			CefV8Value::CreateFunction("getThemes", getThemesHandler);
		api->SetValue("getThemes", getThemesFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		CefRefPtr<CefV8Value> getThemeDataFunc =
			CefV8Value::CreateFunction("getThemeData", getThemeDataHandler);
		api->SetValue(
			"getThemeData", getThemeDataFunc, V8_PROPERTY_ATTRIBUTE_NONE);

		return api;
	}

	bool themeExists(const std::string& fileName) {
		return std::filesystem::exists(path::getThemesDir() / fileName);
	};

	void ThemeCache::initThemes() {
		const auto themesDir = path::getThemesDir();
		if (!std::filesystem::exists(themesDir)) {
			logger.info("Themes directory does not exist, no themes to load");
			return;
		}
		const std::scoped_lock lock(themesMutex);
		const auto iter =
			std::filesystem::recursive_directory_iterator(themesDir);
		const auto beforeSize = themes.size();
		for (const auto& entry : iter) {
			std::error_code errCode;
			if (entry.is_regular_file(errCode)) {
				const auto content = fs::readFile(entry.path());
				themes.insert(std::make_shared<UserTheme>(getThemeInfo(
					content,
					entry.path().lexically_relative(themesDir).string())));
				logger.debug("loaded theme: {}", entry.path().string());
			} else if (entry.is_directory(errCode)) {
				logger.trace("recursing into directory: {}",
							 entry.path().string());
			}
			if (errCode) {
				logger.warn("error iterating over path {}. msg: {}",
							entry.path().string(),
							errCode.message());
			}
		}
		const auto afterSize = themes.size();
		logger.info("Loaded {} themes from {}",
					afterSize - beforeSize,
					themesDir.string());
		logger.trace("exiting theme loading thread");
	}

	std::optional<std::shared_ptr<UserTheme>>
	ThemeCache::getFromName(const std::string& name) const {
		const std::shared_lock lock(themesMutex);
		const auto pos = std::ranges::find(themes, name, &UserTheme::name);
		if (pos != themes.end()) {
			return *pos;
		}
		return {};
	};

	std::optional<std::shared_ptr<UserTheme>>
	ThemeCache::getFromFileName(const std::string& fileName) const {
		const std::shared_lock lock(themesMutex);
		const auto pos =
			std::ranges::find(themes, fileName, &UserTheme::fileName);
		if (pos != themes.end()) {
			return *pos;
		}
		return {};
	};

	void ThemeCache::dispatchThemeUpdate(const std::string& themeName) {
		const auto theme = getFromName(themeName);
		if (!theme) {
			logger.warn("Attempting to dispatch theme update for a theme "
						"that does not exist: {}",
						themeName);
			return;
		}
		dispatchThemeUpdate(path::getThemesDir() / (*theme)->fileName);
	}

	void ThemeCache::dispatchThemeUpdate(const std::filesystem::path& path) {
		const auto relativePath =
			path.lexically_relative(path::getThemesDir()).string();
		auto theme = getFromFileName(relativePath);
		std::scoped_lock lock(themesMutex);

		if (theme) {
			themes.erase(*theme);
		}
		if (!std::filesystem::exists(path)
			|| std::filesystem::is_directory(path)) {
			return;
		}

		const auto content = fs::readFile(path);
		themes.emplace(
			std::make_shared<UserTheme>(getThemeInfo(content, relativePath)));
		CefTaskRunner::GetForThread(CefThreadId::TID_RENDERER)
			->PostTask(TaskCBHandler::Create([relativePath]() {
				if (!themeChangeContext) {
					logger.warn("attempting to dispatch a quick css update "
								"before any listeners are setup");
					return;
				}
				if (!themeChangeContext->IsValid()) {
					logger.error("quick css context is not valid");
					return;
				}
				themeChangeContext->GetTaskRunner()->PostTask(
					TaskCBHandler::Create([relativePath]() -> void {
						for (const auto& callback : themeChangeListeners) {
							if (!callback->IsFunction()) {
								logger.warn("cb in theme change listeners that "
											"is not a function, this "
											"should never happen");
								continue;
							}
							callback->ExecuteFunctionWithContext(
								themeChangeContext,
								nullptr,
								{CefV8Value::CreateString(relativePath)});
						}
					}));
			}));
	}

	std::vector<std::shared_ptr<UserTheme>> ThemeCache::getThemes() const {
		return {themes.begin(), themes.end()};
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
			logger.warn("Attempting to set an empty value for field: {}",
						field);
		}

		if (field == "fileName" || field == "content") {
			logger.warn("Attempting to set the {} of a UserTheme, this "
						"is not allowed. Ignoring.",
						field);
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

	CefRefPtr<CefV8Value> UserTheme::toJSON() const {
		auto obj = CefV8Value::CreateObject(nullptr, nullptr);
		obj->SetValue("fileName",
					  CefV8Value::CreateString(fileName),
					  V8_PROPERTY_ATTRIBUTE_NONE);
		obj->SetValue(
			"name", CefV8Value::CreateString(name), V8_PROPERTY_ATTRIBUTE_NONE);
		obj->SetValue("content",
					  CefV8Value::CreateString(content),
					  V8_PROPERTY_ATTRIBUTE_NONE);
		obj->SetValue("author",
					  CefV8Value::CreateString(author),
					  V8_PROPERTY_ATTRIBUTE_NONE);
		obj->SetValue("description",
					  CefV8Value::CreateString(description),
					  V8_PROPERTY_ATTRIBUTE_NONE);
		if (version) {
			obj->SetValue("version",
						  CefV8Value::CreateString(*version),
						  V8_PROPERTY_ATTRIBUTE_NONE);
		}
		if (license) {
			obj->SetValue("license",
						  CefV8Value::CreateString(*license),
						  V8_PROPERTY_ATTRIBUTE_NONE);
		}
		if (source) {
			obj->SetValue("source",
						  CefV8Value::CreateString(*source),
						  V8_PROPERTY_ATTRIBUTE_NONE);
		}
		if (website) {
			obj->SetValue("website",
						  CefV8Value::CreateString(*website),
						  V8_PROPERTY_ATTRIBUTE_NONE);
		}
		if (invite) {
			obj->SetValue("invite",
						  CefV8Value::CreateString(*invite),
						  V8_PROPERTY_ATTRIBUTE_NONE);
		}
		return obj;
	};

	// from ECMAScript regex
	// /[^\S\r\n]*?\r?(?:\r\n|\n)[^\S\r\n]*?\*[^\S\r\n]?/;
	const static std::basic_regex<char> lineRegex {
		R"([^\S\r\n]*?\r?(?:\r\n|\n)[^\S\r\n]*?\*[^\S\r\n]?)"};
	const static std::basic_regex<char> escapedAtRegex {R"(^\\@)"};

	namespace {
		UserTheme makeHeader(const std::string& fileName,
							 const UserTheme& previousThemeData) {
			UserTheme theme = previousThemeData;
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
			return theme;
		}
	} // namespace

	// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
	UserTheme getThemeInfo(const std::string& css,
						   const std::string& fileName) {
		UserTheme theme;
		theme.content = css;
		if (theme.content.empty()) {
			logger.warn("making theme info for empty css with filename: {}",
						fileName);
			return makeHeader(fileName, theme);
		}
		std::string block;
		{
			const auto first =
				string::split(theme.content, "/**", {.limit = 2});
			if (first.size() < 2) {
				return makeHeader(fileName, theme);
			}
			const auto end = string::split(first[1], "*/", {.limit = 1});
			if (!end.size()) {
				return makeHeader(fileName, theme);
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
				const auto sep = line.find(' ');
				assert(sep != std::string::npos && "TODO: handle this");
				field = line.substr(1, sep - 1);
				accum = line.substr(sep + 1);
			} else {
				std::string value = line;
				if (const auto pos = line.find("\\n");
					pos != std::string::npos) {
					value = value.replace(pos, 2, "\n");
				}
				string::replace(value, escapedAtRegex, "@");
			}
		}
		string::trim(accum);
		theme.set(field, std::move(accum));
		return makeHeader(fileName, theme);
	}

} // namespace Extendify::api::themes
