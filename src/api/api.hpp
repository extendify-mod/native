#pragma once
#include "log/Logger.hpp"

#include <cef_base.h>
#include <cef_callback.h>
#include <cef_v8.h>
#include <cstdint>
#include <include/cef_base.h>
#include <include/internal/cef_ptr.h>
#include <optional>
#include <string>

#define CB_HANDLER_ARGS                                                                   \
	const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, \
		CefRefPtr<CefV8Value>&retval, CefString &exception

namespace Extendify::api {
	extern log::Logger logger;
	void inject(const CefRefPtr<CefV8Context>& context);

	enum V8Type {
		UNDEFINED = 1,
		NULL_TYPE = 2 << 0,
		BOOL = 2 << 1,
		INT = 2 << 2,
		UINT = 2 << 3,
		DOUBLE = 2 << 4,
		DATE = 2 << 5,
		STRING = 2 << 6,
		OBJECT = 2 << 7,
		ARRAY = 2 << 8,
		ARRAY_BUFFER = 2 << 9,
		FUNCTION = 2 << 10,
		PROMISE = 2 << 11,
	};

	/**
	 * @brief gets the type for a v8 value
	 *
	 * @param value a v8 value
	 * @invariant @param value is valid
	 * @return V8Type
	 */
	[[nodiscard]] V8Type getV8Type(const CefRefPtr<CefV8Value>& value);

	/**
	 * @brief Get the type of value
	 *
	 * @param value
	 * @invariant @param value is valid
	 * @return const std::string&
	 */
	[[nodiscard]] std::string getTypeName(const CefRefPtr<CefV8Value>& value);
	[[nodiscard]] constexpr std::string getTypeName(V8Type type);

	struct APIFunction {
		std::string name;
		std::string description;
		std::string path;
		bool allowTrailingArgs = false;
		/**
		 * @brief array of parameter types, bitwise OR'd together
		 * @see V8Type
		 */
		const std::vector<uint64_t> expectedArgs;
		/**
		 * @brief  return type, bitwise OR'd together
		 * @see V8Type
		 */
		std::optional<uint64_t> returnType;
	};

	class APIUsage {
	  public:
		APIUsage();
		// NOLINTNEXTLINE(google-explicit-constructor)
		[[nodiscard]] APIUsage(APIFunction func);

		[[nodiscard]] constexpr std::string getUsage() const noexcept;

		void validateOrThrow(const CefV8ValueList& arguments) const noexcept(false);

		[[nodiscard]] std::optional<std::string>
		validateArgs(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] std::string
		makeActualUsageString(const CefV8ValueList& arguments) const noexcept;

		[[nodiscard]] constexpr static std::string
		makeUsageString(const APIFunction& func) noexcept;

		/**
		 * @brief converts a vector of V8Type to a vector of strings for each type
		 * 
		 * does not accept union types
		 * 
		 * @param types the types to convert
		 * @return std::vector<std::string> 
		 */
		[[nodiscard]] constexpr static std::vector<std::string>
		typesToString(const std::vector<V8Type>& types) noexcept;

		[[nodiscard]] constexpr static std::vector<std::string>
		typesToString(const std::vector<uint64_t>& types) noexcept;

		[[nodiscard]] constexpr static std::string stringifyUnionType(uint64_t type) noexcept;

	  private:
		const std::string usageString;
		const APIFunction func;
	};

	class CBHandler final: public CefV8Handler {
	  public:
		typedef std::function<bool(CB_HANDLER_ARGS)> Callback;

		static CefRefPtr<CBHandler> Create(Callback h);

		bool Execute(CB_HANDLER_ARGS) override;

	  private:
		void setCallback(Callback h);
		Callback handler;
		IMPLEMENT_REFCOUNTING(CBHandler);
	};
} // namespace Extendify::api
