#include "Watcher.hpp"

#include "log/log.hpp"
#include "log/Logger.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#ifdef _WIN32
#include <windows.h>

#include <basetsd.h>
#include <ioapiset.h>
#include <synchapi.h>
#include <winnt.h>

#endif

using namespace Extendify;
using fs::Watcher;

#ifdef _WIN32
static constexpr DWORD events =
	FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE;
#endif

log::Logger Watcher::logger({"Extendify", "fs", "Watcher"});

std::shared_ptr<Watcher> Extendify::fs::Watcher::get() {
	static std::shared_ptr<Watcher> watcher = std::make_shared<Watcher>();
	return watcher;
};

void Watcher::init() {
	if (running) {
		logger.warn("Watcher already running, ignoring it");
		return;
	}
	running = true;
	watchingThread = std::make_unique<std::jthread>([this] { runLoop(); });
#ifdef _WIN32
	eventThread = std::make_unique<std::jthread>([this] { processEvents(); });
#endif
}

#ifdef _WIN32
static constexpr Watcher::Reason reasonFromAction(DWORD action) {
	switch (action) {
		case FILE_ACTION_ADDED:
			return Watcher::Reason::ADDED;
		case FILE_ACTION_REMOVED:
			return Watcher::Reason::REMOVED;
		case FILE_ACTION_MODIFIED:
			return Watcher::Reason::MODIFIED;
		case FILE_ACTION_RENAMED_OLD_NAME:
			return Watcher::Reason::RENAMED_OLD_NAME;
		case FILE_ACTION_RENAMED_NEW_NAME:
			return Watcher::Reason::RENAMED_NEW_NAME;
		default:
			E_ASSERT(false && "Unknown action type");
	}
}
#endif

Watcher::Event::Event(std::filesystem::path path, Reason reason, int watchId):
	Change {std::move(path), reason},
	watchId(watchId) {
}

Watcher::Watcher() {
#ifdef _WIN32
	auto _onChange =
		CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
	if (_onChange == nullptr) {
		logger.error("Error creating IO completion port: {}", GetLastError());
	}
	onChange = _onChange;
	auto _hasEvents = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (_hasEvents == nullptr) {
		logger.error("Error creating event: {}", GetLastError());
	}
	hasEvents = _hasEvents;
#endif
}

Watcher::~Watcher() {
	shutdown = true;
#ifdef _WIN32
	PostQueuedCompletionStatus(onChange, 0, 0, nullptr);
	SetEvent(hasEvents);
#endif
}

int Watcher::addFile(const std::filesystem::path& path,
					 const Callback& callback) {
	if (path.empty()) {
		throw std::invalid_argument("Path cannot be empty");
	}
	if (callback == nullptr) {
		throw std::invalid_argument("Callback cannot be null");
	}
	logger.info("Adding file to watcher: {}", path.string());
	auto dirname = path.parent_path();

#ifdef _WIN32
	if (!dirs.contains(dirname)) {
		dirs.emplace(dirname, std::make_unique<Dir>(dirname, onChange));
		dirs[dirname]->watch();
	}

	return dirs[dirname]->addFile(path, callback);
#endif
}

int Watcher::addDir(const std::filesystem::path& path,
					const Callback& callback) {
	if (path.empty()) {
		throw std::invalid_argument("Path cannot be empty");
	}
	if (callback == nullptr) {
		throw std::invalid_argument("Callback cannot be null");
	}
	logger.info("Adding directory to watcher: {}", path.string());
	if (!dirs.contains(path)) {
		dirs.emplace(path, std::make_unique<Dir>(path, onChange));
		dirs[path]->watch();
	}
	return dirs[path]->addDir(callback);
}

void Watcher::runLoop() {
#ifdef _WIN32
	DWORD bytesTransferred;
	OVERLAPPED* overlapped;
	while (true) {
		// Any valid id that maps to a path
		const std::filesystem::path* dirPath;
		auto ret = GetQueuedCompletionStatus(onChange,
											 &bytesTransferred,
											 (ULONG_PTR*)&dirPath,
											 &overlapped,
											 INFINITE);
		if (shutdown) {
			logger.info("Watcher shutdown requested, exiting loop");
			return;
		}
		if (!ret) {
			logger.error("Error getting queued completion status: {}",
						 GetLastError());
			continue;
		}
		if (dirPath == nullptr) {
			logger.error("dir is null");
			continue;
		}
		if (bytesTransferred == 0) {
			logger.warn("Bytes transferred is 0");
			continue;
		}
		// process the events
		{
			std::scoped_lock lock(pendingEventsMutex, dirsMutex);
			if (!dirs.contains(*dirPath)) {
				logger.warn("No directory registered for {}, ignoring it",
							dirPath->string());
				continue;
			}
			const auto& dir = dirs[*dirPath];
			const auto& data = dir->buf;
			int64_t offset = 0;
			FILE_NOTIFY_INFORMATION* cur = nullptr;
			do {
				cur = (FILE_NOTIFY_INFORMATION*)(data + offset);
				const auto reason = reasonFromAction(cur->Action);
				// the filename is relative, not absolute
				std::filesystem::path filename(dir->baseDir);
				filename /=
					{{cur->FileName, cur->FileNameLength / sizeof(WCHAR)}};
				pendingEvents.emplace_back(
					std::move(filename), reason, dir->baseDir);
				offset += cur->NextEntryOffset;
			} while (cur->NextEntryOffset);
			dir->watch();
		}
		SetEvent(hasEvents);
	}
#endif
}

#ifdef _WIN32

std::atomic_int Watcher::Dir::nextId = 1;

Watcher::Dir::Dir(std::filesystem::path baseDir, HANDLE onChange):
	buf(),
	baseDir(std::move(baseDir)),
	onChange(onChange),
	overlapped() {

	HANDLE _baseDirHandle =
		CreateFile(this->baseDir.string().c_str(),
				   FILE_LIST_DIRECTORY | GENERIC_READ,
				   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				   nullptr,
				   OPEN_EXISTING,
				   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
				   nullptr);

	if (_baseDirHandle == INVALID_HANDLE_VALUE) {
		throw std::runtime_error(std::format("Error opening base dir {}: {}",
											 this->baseDir.string(),
											 GetLastError()));
	}
	baseDirHandle = _baseDirHandle;
	if (nullptr
		== CreateIoCompletionPort(
			baseDirHandle, onChange, (ULONG_PTR) & this->baseDir, 0)) {
		throw std::runtime_error(std::format(
			"Error creating IO completion port: {}", GetLastError()));
	}
}

void Watcher::Dir::watch() {
	memset(buf, 0, sizeof(buf));
	memset(&overlapped, 0, sizeof(overlapped));
	if (!ReadDirectoryChangesW(baseDirHandle,
							   &buf,
							   sizeof(buf),
							   true,
							   events,
							   nullptr,
							   &overlapped,
							   nullptr)) {
		const auto msg =
			std::format("Failed to read directory changes: {}", GetLastError());
		logger.error(msg);
		throw std::runtime_error(msg);
	}
};

int Watcher::Dir::addFile(const std::filesystem::path& path,
						  const Callback& callback) {
	E_ASSERT(!path.empty() && "Path cannot be empty");
	E_ASSERT(callback != nullptr && "Callback cannot be null");
	if (path != baseDir && path.parent_path() != baseDir) {
		throw std::invalid_argument(
			std::format("Path must be a direct child of the base dir or the "
						"base dir itself. Base dir: {}, path: {}",
						baseDir.string(),
						path.string()));
	}
	std::scoped_lock lock(callbacksMutex, idsMutex, pathsMutex);
	auto id = ++nextId;
	if (ids.contains(path)) {
		ids[path].push_back(id);
	} else {
		ids[path] = {id};
	}
	paths[id] = path;
	callbacks[id] = callback;
	return id;
}

int Watcher::Dir::addDir(const Callback& callback) {
	E_ASSERT(callback != nullptr && "Callback cannot be null");
	std::scoped_lock lock(callbacksMutex, idsMutex, pathsMutex);
	auto id = ++nextId;
	if (ids.contains(baseDir)) {
		ids[baseDir].push_back(id);
	} else {
		ids[baseDir] = {id};
	}
	paths[id] = baseDir;
	recursiveCallbacks[id] = callback;
	return id;
}

void Watcher::Dir::removeWatch(int id) {
	{
		std::scoped_lock lock(callbacksMutex);
		if (callbacks.contains(id)) {
			callbacks.erase(id);
		} else {
			logger.warn("No callback registered for id {}, ignoring it", id);
		}
	}
	std::optional<std::filesystem::path> path;
	{
		std::scoped_lock lock(pathsMutex);
		if (paths.contains(id)) {
			path = paths[id];
			paths.erase(id);
		} else {
			logger.warn("No path registered for id {}, ignoring it", id);
		}
	}
	if (path) {
		std::scoped_lock lock(idsMutex);
		if (ids.contains(*path)) {
			auto& vec = ids[*path];
			if (const auto pos = std::find(vec.begin(), vec.end(), id);
				pos != vec.end()) {
				vec.erase(pos);
			} else {
				logger.warn("No id {} registered for path {}, ignoring it",
							id,
							path->string());
			}
		}
	}
}

void Watcher::processEvents() {
	std::deque<std::pair<std::unique_ptr<Event>, Callback>> toProcess;
	while (true) {
		{
			if (!toProcess.empty()) {
				constexpr static auto msg =
					"Pending events not empty, This should never happen";
				logger.error(msg);
				throw std::runtime_error(msg);
			}
			std::scoped_lock lock(pendingEventsMutex, dirsMutex);
			for (const auto& [path, reason, dirpath] : pendingEvents) {
				if (dirs.contains(dirpath)) {
					const auto& dir = dirs[dirpath];
					std::scoped_lock lock(dir->idsMutex);
					if (dir->ids.contains(path)) {
						const auto& vec = dir->ids[path];
						for (const auto& id : vec) {
							toProcess.emplace_back(
								std::make_unique<Event>(path, reason, id),
								dir->callbacks[id]);
						}
					}
					for (const auto& [id, callback] : dir->recursiveCallbacks) {
						toProcess.emplace_back(
							std::make_unique<Event>(path, reason, id),
							callback);
					}
				} else {
					logger.warn("Recieved event for path {} but no directory "
								"registered for it, "
								"ignoring it",
								path.string());
				}
			}
			pendingEvents.clear();
		}
		int processedEvents = 0;
		while (!toProcess.empty()) {
			auto _entry = toProcess.begin();
			auto event = std::move(_entry->first);
			auto callback = _entry->second;
			E_ASSERT(callback != nullptr && "Callback cannot be null");
			try {
				logger.trace("dispatching fs watcher event: {}", event);
				callback(std::make_unique<Event>(*event));
				processedEvents++;
			} catch (std::exception& e) {
				logger.error(
					"Error occurred while processing {}, {}", event, e.what());
			}
			toProcess.erase(_entry);
		}
		logger.debug("Processed {} events", processedEvents);
		// wait for next event
		WaitForSingleObjectEx(hasEvents, INFINITE, true);
		if (shutdown) {
			logger.info("Watcher shutdown requested, exiting event processing");
			return;
		}
	}
}

#elif defined(__linux__)
#warning TODO
#endif
