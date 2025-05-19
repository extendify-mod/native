#pragma once

#include "log/Logger.hpp"
#include <cef_callback.h>
#include <cef_v8.h>

namespace Extendify::api::quickCss {
    extern log::Logger logger;

    CefRefPtr<CefV8Value> makeApi();
}