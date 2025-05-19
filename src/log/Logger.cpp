#include "Logger.hpp"

#include "path/path.hpp"
#include "util/string.hpp"

#include <algorithm>
#include <initializer_list>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace Extendify::log;

#pragma region ctor

Logger::Logger(const std::initializer_list<std::string>&& names):
	logger(makeFullLoggerName(names),
		   {
			   getDefaultFileSink(),
			   std::make_shared<spdlog::sinks::stderr_color_sink_mt>(),
		   }),
	_names(names) {
		set_level(defaultLevel);
		addToAll();
}

Logger::Logger(const std::initializer_list<std::string>&& names, spdlog::sinks_init_list sinks):
	logger(makeFullLoggerName(names), sinks) {
		set_level(defaultLevel);
		addToAll();
}

Logger::Logger(const Logger& other):
	logger(other) {
		addToAll();
}

Logger::Logger(logger&& other) noexcept:
	logger(other) {
		addToAll();
}

Logger::~Logger() {
	// remove from allLoggers
	removeFromAll();
}

#pragma endregion

void Logger::addSink(const std::shared_ptr<spdlog::sinks::sink>& sink) {
	sinks().push_back(sink);
}

std::string Logger::makeFullLoggerName(const std::initializer_list<std::string>& names) {
	return util::string::join(names.begin(), names.end(), "/");
}

std::shared_ptr<spdlog::sinks::sink> Logger::getDefaultFileSink() {
	// TODO: use event handlers to handle opening the file with more than one process
	static std::shared_ptr<spdlog::sinks::sink> sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		(path::getLogDir() / "native.log").string(), 1048576 * 5, 1000, false);

	return sink;
}

std::vector<Logger*> Logger::allLoggers;

void Logger::setLevelForAll(spdlog::level::level_enum level) {
	for (const auto logger : allLoggers) {
		logger->set_level(level);
	}
}

void Logger::addToAll() {
	if(std::find(allLoggers.begin(), allLoggers.end(), this) != allLoggers.end()) {
		throw std::runtime_error("Logger already in allLoggers, this should not happen");
	}
	allLoggers.push_back(this);
}

void Logger::removeFromAll() {
	const auto pos = std::find(allLoggers.begin(), allLoggers.end(), this);
	if(pos == allLoggers.end()) {
		throw std::runtime_error("Logger not in allLoggers, this should not happen");
	}
	allLoggers.erase(pos);
}
