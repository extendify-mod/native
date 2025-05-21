#pragma once
#include <filesystem>
#include <functional>

namespace Extendify::fs {
	class Watcher final {
	  public:
		typedef std::function<void()> Callback;
		/**
		 * @brief Adds a file to the watcher
		 *
		 * @param path the file path to be watched
		 * @param callback the callback to be called when the file is changed/created/deleted
		 * @return a unique, non-negative, non-zero id for this file/callback pair, use it to remove the watcher
		 */
		int addFile(std::filesystem::path path, Callback callback);

	  private:
		/**
		 * @brief id to callback
		 */
		std::unordered_map<int, Callback> callbacks;
        /**
         * @brief id to path
         */
		std::unordered_map<int, std::filesystem::path> paths;
        /**
         * @brief path to ids
         */
		std::unordered_map<std::filesystem::path, std::vector<int>> ids;
        /**
         * @brief used to generate the next id, starts at 1
         *
         * increment before returning
         */
		static int nextId;
	};
} // namespace Extendify::fs
