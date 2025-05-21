#pragma once
#include "log/Logger.hpp"

#include <atomic>
#include <deque>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>

#ifdef _WIN32
#include <Windows.h>

#include <winnt.h>

#endif

namespace Extendify::fs {
	class Watcher final {
	  public:
		enum class Reason {
			ADDED,
			REMOVED,
			MODIFIED,
			RENAMED_OLD_NAME,
			RENAMED_NEW_NAME,
		};

		struct Change {
			const std::filesystem::path path;
			const Reason reason;
		};

		struct Event: Change {
			Event(std::filesystem::path path, Reason reason, int watchId);
			const int watchId;
		};

		Watcher();
		~Watcher();
		typedef std::function<void(std::unique_ptr<const Event>)> Callback;
		/**
		 * @brief Adds a file to the watcher
		 *
		 * @param path the file path to be watched
		 * @param callback the callback to be called when the file is changed/created/deleted
		 * @return a unique, non-negative, non-zero id for this file/callback pair, use it to remove
		 * the watcher
		 */
		int addFile(const std::filesystem::path& path, const Callback& callback);

		/**
		 * @brief Removes a file from the watcher
		 *
		 * @param id the id of the file/callback pair to be removed
		 */
		void removeFile(int id);

		/**
		 * @brief starts watching for changes
		 *
		 * must be called after cef_initialize due to threads created prior to
		 * cef_initialize being killed for sandbox reasons
		 */
		void init();

	  private:
		static log::Logger logger;

		class Dir {
		  public:
			char buf[2 << 12];
			int addFile(const std::filesystem::path& path, const Callback& callback);
			void removeFile(int id);
			void watch();
			Dir() = delete;
			Dir(std::filesystem::path baseDir, HANDLE onChange);

		  private:
			const std::filesystem::path baseDir;
			HANDLE baseDirHandle;
			HANDLE onChange;
			OVERLAPPED overlapped;
			std::mutex pathsMutex;
			std::unordered_map<int, std::filesystem::path> paths;
			std::mutex idsMutex;
			std::unordered_map<std::filesystem::path, std::vector<int>> ids;
			std::mutex callbacksMutex;
			std::unordered_map<int, Callback> callbacks;
			/**
			 * @brief used to generate the next id, starts at 1
			 *
			 * increment before returning
			 */
			static std::atomic_int nextId;
			friend class Watcher;
		};

		std::mutex dirsMutex;
		/**
		 * @brief path to ids
		 */
		std::unordered_map<std::filesystem::path, std::unique_ptr<Dir>> dirs;

		enum class TransactionType {
			ADD,
			REMOVE
		};

		typedef std::tuple<int, TransactionType> PendingTransaction;
		/**
		 * @brief pending transactions
		 *
		 * used before the watching thread is started
		 * and while the thread is running
		 */
		std::deque<PendingTransaction> pendingTransactions;

		std::mutex pendingEventsMutex;
		std::deque<std::pair<std::filesystem::path, Reason>> pendingEvents;

		void runLoop();
		void processEvents();
#ifdef _WIN32
		/**
		 * @brief triggered when a file is changed
		 */
		HANDLE onChange;
		/**
		 * @brief triggered when new events are available
		 */
		HANDLE hasEvents;
#elif defined(__linux__)
#warning TODO
#endif
	};
} // namespace Extendify::fs

template<>
struct std::formatter<Extendify::fs::Watcher::Event> {
  private:
  public:
	bool ok = false;

	template<typename ParseCtx>
	ParseCtx::iterator parse(ParseCtx& ctx) {
		// no format specifiers
		if (ctx.begin() == ctx.end() || *ctx.begin() == '}') {
			ok = true;
			return ctx.end();
		}
		throw std::format_error(
			"No format specifiers are supported for Extendify::fs::Watcher::Event");
	}

	template<typename FmtCtx>
	FmtCtx::iterator format(const Extendify::fs::Watcher::Event& event, FmtCtx& ctx) const {
		if (!ok) {
			unreachable();
		}
		return std::format_to(ctx.out(),
							  "Event: {{path: {}, reason: {}, watchId: {}}}",
							  event.path.string(),
							  event.reason,
							  event.watchId);
	}
};

template<>
struct std::formatter<std::unique_ptr<Extendify::fs::Watcher::Event>> {
  private:
  public:
	bool ok = false;

	template<typename ParseCtx>
	constexpr ParseCtx::iterator parse(ParseCtx& ctx) {
		// no format specifiers
		if (ctx.begin() == ctx.end() || *ctx.begin() == '}') {
			ok = true;
			return ctx.end();
		}
		throw std::format_error(
			"No format specifiers are supported for Extendify::fs::Watcher::Event");
	}

	template<typename FmtCtx>
	FmtCtx::iterator format(const std::unique_ptr<Extendify::fs::Watcher::Event>& event,
							FmtCtx& ctx) const {
		if (!ok) {
			unreachable();
		}
		return std::format_to(ctx.out(),
							  "Event: {{path: {}, reason: {}, watchId: {}}}",
							  event->path.string(),
							  event->reason,
							  event->watchId);
	}
};

template<>
struct std::formatter<Extendify::fs::Watcher::Reason> {
  private:
  public:
	bool ok = false;

	template<typename ParseCtx>
	constexpr ParseCtx::iterator parse(ParseCtx& ctx) {
		// no format specifiers
		if (ctx.begin() == ctx.end() || *ctx.begin() == '}') {
			ok = true;
			return ctx.end();
		}
		throw std::format_error(
			"No format specifiers are supported for Extendify::fs::Watcher::Event");
	}

	template<typename FmtCtx>
	FmtCtx::iterator format(const Extendify::fs::Watcher::Reason& event,
							FmtCtx& ctx) const {
		if (!ok) {
			unreachable();
		}
		switch (event) {
			case Extendify::fs::Watcher::Reason::ADDED:
				return std::format_to(ctx.out(), "ADDED");
			case Extendify::fs::Watcher::Reason::REMOVED:
				return std::format_to(ctx.out(), "REMOVED");
			case Extendify::fs::Watcher::Reason::MODIFIED:
				return std::format_to(ctx.out(), "MODIFIED");
			case Extendify::fs::Watcher::Reason::RENAMED_OLD_NAME:
				return std::format_to(ctx.out(), "RENAMED_OLD_NAME");
			case Extendify::fs::Watcher::Reason::RENAMED_NEW_NAME:
				return std::format_to(ctx.out(), "RENAMED_NEW_NAME");
		}
		assert(false && "Unknown reason type");
		unreachable();
	}
};