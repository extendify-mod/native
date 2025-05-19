#pragma once
#include "log/Logger.hpp"

#include <cef_base.h>
#include <cef_callback.h>
#include <cef_v8.h>
#include <include/cef_base.h>

#define CB_HANDLER_ARGS                                                                   \
	const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, \
		CefRefPtr<CefV8Value>&retval, CefString &exception

namespace Extendify::api {
	extern log::Logger logger;
	void inject(const CefRefPtr<CefV8Context>& context);

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
