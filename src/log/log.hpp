#pragma once
#include "Logger.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {
	[[gnu::always_inline]] [[noreturn]] inline bool e_abort(const char* msg, const char* file,
															int line, const char* func) {
		std::cerr << file << ":" << line << ": " << func << ": Assertion `" << msg << "` failed."
				  << std::endl;
		std::abort();
	}

} // namespace
#ifdef _WIN32
#define E_ASSERT(x) (void)(!!(x) || (e_abort(#x, __FILE__, __LINE__, __func__)))
#endif

namespace Extendify {
	extern log::Logger logger;
}; // namespace Extendify
