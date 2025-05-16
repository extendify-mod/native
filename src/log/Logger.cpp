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

std::string Logger::makeFullLoggerName(const std::initializer_list<std::string>& names) {
	return util::string::join(names.begin(), names.end(), "/");
}

std::shared_ptr<spdlog::sinks::sink> Logger::getDefaultFileSink() {
	static std::shared_ptr<spdlog::sinks::sink> sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		(path::getLogDir() / "latest.log").string(), SIZE_MAX - 5000, 200000 /* hardcoded limit in spdlog */, true);

	return sink;
}
