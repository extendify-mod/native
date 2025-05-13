#pragma once

#include <initializer_list>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/sink.h>

namespace Extendify::log {
	class Logger final: public spdlog::logger {
	public:
		Logger(const std::initializer_list<std::string>&& names);

		Logger(const std::initializer_list<std::string>&& names, spdlog::sinks_init_list sinks);

		~Logger() override = default;

		Logger(const Logger& other);

		explicit Logger(logger&& other) noexcept;

		void addSink(const std::shared_ptr<spdlog::sinks::sink>& sink);

	private:
		static std::string makeFullLoggerName(const std::initializer_list<std::string>& names);

		static std::shared_ptr<spdlog::sinks::sink> getDefaultFileSink();

		std::vector<std::string> _names;
	};

} // namespace Extendify::log
