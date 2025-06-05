#pragma once

#include <functional>
#include <initializer_list>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/sink.h>
#include <unordered_set>

namespace Extendify::log {
	class Logger final: public spdlog::logger {
	  public:
		Logger(const std::initializer_list<std::string>&& names);

		Logger(const std::initializer_list<std::string>&& names,
			   spdlog::sinks_init_list sinks);

		~Logger() override;

		Logger(const Logger& other);

		explicit Logger(logger&& other) noexcept;

		void addSink(const std::shared_ptr<spdlog::sinks::sink>& sink);

		static void setLevelForAll(spdlog::level::level_enum level);

	  private:
		static std::string
		makeFullLoggerName(const std::initializer_list<std::string>& names);

		static std::shared_ptr<spdlog::sinks::sink> getDefaultFileSink();

		static std::vector<Logger*> allLoggers;

		static const spdlog::level::level_enum defaultLevel =
			spdlog::level::trace;

		void addToAll();

		void removeFromAll();

		std::vector<std::string> _names;
	};

} // namespace Extendify::log
