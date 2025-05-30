#pragma once

#include <cef_v8.h>
#include <functional>

#define CB_HANDLER_ARGS                                                \
	const CefString &name, CefRefPtr<CefV8Value> object,               \
		const CefV8ValueList &arguments, CefRefPtr<CefV8Value>&retval, \
		CefString &exception

namespace Extendify::api::util {
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
} // namespace Extendify::api::util
