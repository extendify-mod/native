#include "quickCss.hpp"

#include "log/Logger.hpp"

#include <cef_v8.h>
#include <cef_base.h>

using namespace Extendify;
using namespace api;

namespace Extendify::api::quickCss {
	log::Logger logger({"Extendify", "api", "quickCss"});
} // namespace quickCss


CefRefPtr<CefV8Value> quickCss::makeApi() {
    CefRefPtr<CefV8Value> api = CefV8Value::CreateObject(nullptr, nullptr);
    return api;
}
