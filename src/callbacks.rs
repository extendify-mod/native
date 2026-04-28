use crate::cef::utils::stoc;
use crate::cef::{_cef_settings_t, _cef_v8_context_t};
use crate::log;
use std::sync::atomic::{AtomicBool, Ordering::Relaxed};
use ureq;

const SCRIPT_URL: &str =
    "https://github.com/extendify-mod/extendify/releases/download/artifacts/extendify.js";
const STYLE_URL: &str =
    "https://github.com/extendify-mod/extendify/releases/download/artifacts/extendify.css";

pub fn on_entrypoint(settings: *mut _cef_settings_t) {
    unsafe {
        if (*settings).remote_debugging_port == 0 {
            (*settings).remote_debugging_port = 9229;
            log("Enabled remote debugging on port 9229");
        }
    }
}

static INJECTED: AtomicBool = AtomicBool::new(false);

pub fn on_context(context: *mut _cef_v8_context_t) {
    if INJECTED.load(Relaxed) {
        return;
    }

    log("Creating global Extendify");

    unsafe {
        (*context).enter.unwrap()(context);
        let frame = (*context).get_frame.unwrap()(context);

        if let Some(script) = get(SCRIPT_URL) {
            (*frame).execute_java_script.unwrap()(frame, stoc(script), stoc("extendify_script"), 0);

            log("Injected script");
        }

        if let Some(style) = get(STYLE_URL) {
            (*frame).execute_java_script.unwrap()(
                frame,
                stoc(include_str!("./inject/styles.js").replace("{{style}}", &style)),
                stoc("extendify_styles"),
                0,
            );

            log("Injected styles");
        }

        (*context).exit.unwrap()(context);
    }

    INJECTED.store(true, Relaxed);
}

fn get(url: &str) -> Option<String> {
    match ureq::get(url).call() {
        Ok(mut response) => match response.body_mut().read_to_string() {
            Ok(body) => {
                return Some(body);
            }
            Err(e) => {
                log(format!("Body failed {e}"));
            }
        },
        Err(e) => {
            log(format!("Call failed {e}"));
        }
    }

    None
}
