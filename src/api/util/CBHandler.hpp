#pragma once

#include <cef_base.h>
#include <cef_v8.h>
#include <functional>
#include <internal/cef_ptr.h>
#include <internal/cef_string.h>


#define CB_HANDLER_ARGS                                                \
	const CefString &name, CefRefPtr<CefV8Value> object,               \
		const CefV8ValueList &arguments, CefRefPtr<CefV8Value>&retval, \
		CefString &exception

namespace Extendify::api::util {
	class CBHandler final: public CefV8Handler {
	  public:
		using Callback = std::function<bool(CB_HANDLER_ARGS)>;

		static CefRefPtr<CBHandler> Create(Callback callback);

		bool Execute(CB_HANDLER_ARGS) override;

	  private:
		void setCallback(Callback callback);
		Callback handler;
		IMPLEMENT_REFCOUNTING(CBHandler);
	};
} // namespace Extendify::api::util
