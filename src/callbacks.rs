use crate::cef::{_cef_settings_t, _cef_v8_context_t};
use crate::log;

pub fn on_entrypoint(settings: *mut _cef_settings_t) {
    unsafe {
        if (*settings).remote_debugging_port == 0 {
            (*settings).remote_debugging_port = 9229;
            log("Enabled remote debugging on port 9229");
        }
    }
}

pub fn on_context(context: *mut _cef_v8_context_t) {
    log("Creating global Extendify");
}
