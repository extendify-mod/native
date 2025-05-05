#include "Logger.hpp"

#include "path/path.hpp"
#include "util/string.hpp"

#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace Extendify::log;

#pragma region ctor

Logger::Logger(const std::initializer_list<std::string>&& names):
	logger(makeFullLoggerName(names)),
	_names(names) {
	sinks().push_back(getDefaultFileSink());
	sinks().push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
}

Logger::Logger(const std::initializer_list<std::string>&& names, spdlog::sinks_init_list sinks):
	logger(makeFullLoggerName(names), sinks) {
}

Logger::Logger(const Logger& other):
	logger(other) {
}

Logger::Logger(logger&& other) noexcept:
	logger(other) {
}

#pragma endregion

void Logger::addSink(const std::shared_ptr<spdlog::sinks::sink>& sink) {
	sinks().push_back(sink);
}

#pragma region log and throw

void Logger::logAndThrow(spdlog::log_clock::time_point log_time, spdlog::source_loc loc, spdlog::level::level_enum lvl,
						 spdlog::string_view_t msg) {
	log(log_time, loc, lvl, msg);
	flush();

	throw std::runtime_error(std::string(msg.data(), msg.size()));
}

void Logger::logAndThrow(spdlog::source_loc loc, spdlog::level::level_enum lvl, spdlog::string_view_t msg) {
	log(loc, lvl, msg);
	flush();

	throw std::runtime_error(std::string(msg.data(), msg.size()));
}

void Logger::logAndThrow(spdlog::level::level_enum lvl, spdlog::string_view_t msg) {
	log(lvl, msg);
	flush();

	throw std::runtime_error(std::string(msg.data(), msg.size()));
}

#pragma endregion

std::string Logger::makeFullLoggerName(const std::initializer_list<std::string>& names) {
	return util::string::join(names.begin(), "/");
}

std::shared_ptr<spdlog::sinks::sink> Logger::getDefaultFileSink() {
	static std::shared_ptr<spdlog::sinks::sink> sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		(path::getLogDir() / "latest.log").string(), SIZE_MAX, SIZE_MAX, true);

	return sink;
}
