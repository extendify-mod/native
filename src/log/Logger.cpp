#include "Logger.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/sink.h>

using namespace Extendify::log;

Logger::Logger(const std::initializer_list<std::string>&& names):
	logger(makeFullLoggerName(names)),
	_names(names) {
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
	std::stringstream res;
	for (const auto& name : names) {
		res << '[' << name << ']';
	}
	return res.str();
}

std::shared_ptr<spdlog::sinks::sink> Logger::getDefaultFileSink() {
	static std::shared_ptr<spdlog::sinks::sink> sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>();
}
