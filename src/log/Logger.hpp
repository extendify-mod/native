#pragma once

#include <initializer_list>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/sink.h>

#define CREATE_LOG_OVERRIDE1(name, ret, ...)                                    \
	template<typename... Args>                                                  \
	ret name(__VA_ARGS__::spdlog::format_string_t<Args...> fmt, Args&&... args)
#define CREATE_LOG_OVERRIDE2(name, ret, ...)                                       \
	template<typename... Args>                                                     \
	ret name(__VA_ARGS__::spdlog::source_loc loc, ::spdlog::level::level_enum lvl, \
			 ::spdlog::format_string_t<Args...> fmt, Args&&... args)
#define CREATE_LOG_OVERRIDE3(name, ret, ...)                                                                      \
	template<typename... Args>                                                                                    \
	ret name(__VA_ARGS__::spdlog::level::level_enum lvl, ::spdlog::format_string_t<Args...> fmt, Args&&... args);
#define CREATE_LOG_OVERRIDE4(name, ret, ...)                            \
	template<typename T>                                                \
	ret name(__VA_ARGS__::spdlog::level::level_enum lvl, const T& msg);
#define CREATE_LOG_OVERRIDE5(name, ret, ...)                                                                     \
	template<class T, typename ::std::enable_if<!::spdlog::is_convertible_to_any_format_string<const T&>::value, \
												int>::type = 0>                                                  \
	ret name(__VA_ARGS__::spdlog::source_loc loc, ::spdlog::level::level_enum lvl, const T& msg)
// #define CREATE_LOG_OVERRIDE6(name, ret, ...)                                                \
// 	ret name(__VA_ARGS__::spdlog::log_clock::time_point log_time, ::spdlog::source_loc loc, \
// 			 ::spdlog::level::level_enum lvl, ::spdlog::string_view_t msg)
#define CREATE_LOG_OVERRIDE7(name, ret, ...)                                                                     \
	ret name(__VA_ARGS__::spdlog::source_loc loc, ::spdlog::level::level_enum lvl, ::spdlog::string_view_t msg)
#define CREATE_LOG_OVERRIDE6(name, ret, ...) CREATE_LOG_OVERRIDE7(name, ret, __VA_ARGS__ ::spdlog::log_clock::time_point log_time,)
#define CREATE_LOG_OVERRIDE8(name, ret, ...)                                          \
	ret name(__VA_ARGS__::spdlog::level::level_enum lvl, ::spdlog::string_view_t msg)

namespace Extendify::log {
	class Logger final: public spdlog::logger {
	  public:
		Logger(const std::initializer_list<std::string>&& names);
		Logger(const std::initializer_list<std::string>&& names, spdlog::sinks_init_list sinks);
		~Logger() override = default;
		Logger(const Logger& other);
		explicit Logger(logger&& other) noexcept;

		void addSink(const std::shared_ptr<spdlog::sinks::sink>& sink);

#pragma region Log and throw
		CREATE_LOG_OVERRIDE1(logAndThrow, [[noreturn]] void);
		CREATE_LOG_OVERRIDE2(logAndThrow, [[noreturn]] void);
		CREATE_LOG_OVERRIDE3(logAndThrow, [[noreturn]] void);
		CREATE_LOG_OVERRIDE4(logAndThrow, [[noreturn]] void);
		CREATE_LOG_OVERRIDE5(logAndThrow, [[noreturn]] void);
		CREATE_LOG_OVERRIDE6(logAndThrow, [[noreturn]] void, int foo,);
		CREATE_LOG_OVERRIDE7(logAndThrow, [[noreturn]] void);
		CREATE_LOG_OVERRIDE8(logAndThrow, [[noreturn]] void);

#pragma endregion

	  private:
		static std::string makeFullLoggerName(const std::initializer_list<std::string>& names);
		static std::shared_ptr<spdlog::sinks::sink> getDefaultFileSink();
		std::vector<std::string> _names;
		template<typename... Args>
		std::string makeThrowMessage(spdlog::string_view_t fmt, Args&&... args);
	};

#pragma region Log and throw impl

	template<typename... Args>
	void Logger::logAndThrow(spdlog::format_string_t<Args...> fmt, Args&&... args) {
		logAndThrow(spdlog::level::critical, fmt, args...);
	}

	template<typename... Args>
	[[noreturn]] void Logger::logAndThrow(spdlog::source_loc loc, spdlog::level::level_enum lvl,
										  spdlog::format_string_t<Args...> fmt, Args&&... args) {
		log(loc, lvl, fmt, args...);
		flush();

		throw std::runtime_error(makeThrowMessage(fmt, args...));
	}

	template<typename... Args>
	[[noreturn]] void Logger::logAndThrow(spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt,
										  Args&&... args) {
		log(lvl, fmt, args...);
		flush();

		throw std::runtime_error(makeThrowMessage(fmt, args...));
	}

	template<typename T>
	void Logger::logAndThrow(spdlog::level::level_enum lvl, const T& msg) {
		log(lvl, msg);
		flush();

		throw std::runtime_error(makeThrowMessage("{}", msg));
	}

	template<class T, typename std::enable_if<!spdlog::is_convertible_to_any_format_string<const T&>::value, int>::type>
	[[noreturn]] void Logger::logAndThrow(spdlog::source_loc loc, spdlog::level::level_enum lvl, const T& msg) {
		log(loc, lvl, msg);
		flush();

		throw std::runtime_error(makeThrowMessage("{}", msg));
	}

	template<typename... Args>
	std::string Logger::makeThrowMessage(const spdlog::string_view_t fmt, Args&&... args) {
		spdlog::memory_buf_t buf;
		spdlog::fmt_lib::vformat_to(spdlog::fmt_lib::appender(buf), fmt, fmt::make_format_args(args...));
		return {buf.data(), buf.size()};
	}

#pragma endregion
} // namespace Extendify::log
