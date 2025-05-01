#pragma once
#include "spdlog/common.h"
#include <initializer_list>
#include <spdlog/spdlog.h>

namespace Extendify {
	namespace log {
		class Logger: public spdlog::logger { 
            public:
                Logger(std::initializer_list<std::string> names);
                Logger(std::initializer_list<std::string> names, spdlog::sinks_init_list sinks);
        };
	} // namespace log
} // namespace Extendify
