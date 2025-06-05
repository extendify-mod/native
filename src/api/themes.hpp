#pragma once

#include "log/Logger.hpp"

#include <cef_callback.h>
#include <cef_v8.h>
#include <filesystem>
#include <shared_mutex>
#include <unordered_set>

namespace Extendify::api::themes {
	extern log::Logger logger;
	struct UserTheme;

	[[nodiscard]] CefRefPtr<CefV8Value> makeApi();

	[[nodiscard]] bool themeExists(const std::string& fileName);

	class ThemeCache final {
	  public:
		ThemeCache() = default;
		ThemeCache(const ThemeCache&) = delete;
		ThemeCache& operator=(const ThemeCache&) = delete;
		ThemeCache(ThemeCache&&) noexcept = delete;
		ThemeCache& operator=(ThemeCache&&) = delete;
		~ThemeCache() = default;

		void initThemes();

		[[nodiscard]] std::optional<std::shared_ptr<UserTheme>>
		getFromName(const std::string& name) const;

		[[nodiscard]] std::optional<std::shared_ptr<UserTheme>>
		getFromFileName(const std::string& fileName) const;

		void dispatchThemeUpdate(const std::string& themeName);

		void dispatchThemeUpdate(const std::filesystem::path& path);

		[[nodiscard]] std::vector<std::shared_ptr<UserTheme>> getThemes() const;

	  private:
		mutable std::shared_mutex themesMutex;
		std::unordered_set<std::shared_ptr<UserTheme>> themes;
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
	struct UserTheme {
		void set(const std::string& field, std::string value);
		[[nodiscard]] CefRefPtr<CefV8Value> toJSON() const;
		std::string fileName;
		std::string name;
		std::string content;
		std::string author =
			"Unknown Author"; // Default to Unknown Author if not specified
		std::string description =
			"An Extendify Theme"; // Default to An Extendify Theme if not
								  // specified
		std::optional<std::string> version;
		std::optional<std::string> license;
		std::optional<std::string> source;
		std::optional<std::string> website;
		std::optional<std::string> invite;
	};

	[[nodiscard]] UserTheme getThemeInfo(const std::string& css,
										 const std::string& fileName);
} // namespace Extendify::api::themes
