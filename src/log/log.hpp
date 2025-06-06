#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

[[gnu::always_inline]] [[noreturn]] inline void
e_abort(const char* msg, const char* file, int line, const char* func) {
	// NOLINTNEXTLINE
	fprintf(stderr, "%s:%d:%s: Assertion `%s` failed.", file, line, func, msg);
	abort();
}

#ifdef _WIN32
#define E_ASSERT(x)                                                         \
	(void)(!!(x) || (e_abort(#x, __PRETTY_FUNCTION__, __LINE__, __func__)))
#elif defined(__linux__)
#ifdef NDEBUG
// assert copied from assert.h
// ----
#if defined __has_builtin
#if __has_builtin(__builtin_FILE)
#define __ASSERT_FILE __builtin_FILE()
#define __ASSERT_LINE __builtin_LINE()
#endif
#endif
#if !defined __ASSERT_FILE
#define __ASSERT_FILE __FILE__
#define __ASSERT_LINE __LINE__
#endif
// ----
#if defined __cplusplus ? __GNUC_PREREQ(2, 6) : __GNUC_PREREQ(2, 4)
#define __ASSERT_FUNCTION __extension__ __PRETTY_FUNCTION__
#else
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#define __ASSERT_FUNCTION __func__
#else
#define __ASSERT_FUNCTION ((const char*)0)
#endif
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* This prints an "Assertion failed" message and aborts.  */
void __assert_fail(const char* __assertion, const char* __file,
				   unsigned int __line, const char* __function) __THROW
	__attribute__((__noreturn__));
#ifdef __cplusplus
}
#endif
#endif
#ifdef __cplusplus
#define E_ASSERT(expr)                                                  \
	(static_cast<bool>(expr)                                            \
		 ? void(0)                                                      \
		 : __assert_fail(                                               \
			   #expr, __ASSERT_FILE, __ASSERT_LINE, __ASSERT_FUNCTION))
#else
#define E_ASSERT(expr)                                                     \
	((expr) ? (void)(0)                                                    \
			: __assert_fail(#expr, __FILE__, __LINE__, __ASSERT_FUNCTION))
#endif

#endif
