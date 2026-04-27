use crate::cef::{
    _cef_settings_t, _cef_v8_context_t, cef_app_t, cef_browser_t, cef_frame_t, cef_main_args_t,
    cef_render_process_handler_t,
};
use minhook::MinHook;
use std::ffi::{c_int, c_void};
use std::fmt::Display;
use windows_sys::Win32::Foundation::HINSTANCE;
use windows_sys::Win32::System::Diagnostics::Debug::OutputDebugStringA;
use windows_sys::Win32::System::LibraryLoader::{DisableThreadLibraryCalls, LoadLibraryW};
use windows_sys::Win32::System::SystemServices::DLL_PROCESS_ATTACH;

mod version_reimpl;

// You have to use smth like DebugView to see the logs
// because the renderer is sandboxed so we can't
// write to the log file from that process.
fn log<T: Display>(msg: T) {
    crate::log(&msg);

    let s = format!("{}\0", msg);
    unsafe {
        OutputDebugStringA(s.as_ptr());
    }
}

#[unsafe(no_mangle)]
pub extern "system" fn DllMain(
    hinst: HINSTANCE,
    fdw_reason: u32,
    _lpv_reserved: *mut c_void,
) -> i32 {
    match fdw_reason {
        DLL_PROCESS_ATTACH => unsafe {
            DisableThreadLibraryCalls(hinst);

            init_hooks();

            return 1;
        },
        _ => {}
    }

    0 // Returning 0 here so we don't even have to forward any calls. Might be wrong tho.
}

type CefInitializeFn = unsafe extern "C" fn(
    *const cef_main_args_t,
    *mut _cef_settings_t,
    *mut cef_app_t,
    *mut c_void,
) -> c_int;
static mut CEF_INITIALIZE_OG: Option<CefInitializeFn> = None;

type CefProcessFn =
    unsafe extern "C" fn(*const cef_main_args_t, *mut cef_app_t, *mut c_void) -> c_int;
static mut CEF_PROCESS_OG: Option<CefProcessFn> = None;

type OnContextCreatedFn = unsafe extern "C" fn(
    *mut cef_render_process_handler_t,
    *mut cef_browser_t,
    *mut cef_frame_t,
    *mut _cef_v8_context_t,
);
static mut ON_CONTEXT_CREATED_OG: Option<OnContextCreatedFn> = None;

fn init_hooks() {
    let name: Vec<u16> = "libcef.dll"
        .encode_utf16()
        .chain(std::iter::once(0))
        .collect();
    unsafe { LoadLibraryW(name.as_ptr()) };

    log("Initializing hooks");

    unsafe {
        match MinHook::create_hook_api("libcef.dll", "cef_initialize", cef_initialize_hook as _) {
            Ok(original) => {
                CEF_INITIALIZE_OG = Some(std::mem::transmute(original));
                log("Created cef_initialize hook");
            }
            Err(e) => {
                log(format!("Couldn't create cef_initialize hook {e}"));
            }
        }
        match MinHook::create_hook_api("libcef.dll", "cef_execute_process", cef_process_hook as _) {
            Ok(original) => {
                CEF_PROCESS_OG = Some(std::mem::transmute(original));
                log("Created cef process hook");
            }
            Err(e) => {
                log(format!("Couldn't create cef process hook {e}"));
            }
        }

        if let Err(e) = MinHook::enable_all_hooks() {
            log(format!("Couldn't enable hooks {e}"));
        }
    }
}

fn create_context_hook(app: *mut cef_app_t) {
    if app.is_null() {
        return;
    }

    unsafe {
        let rph = (*app).get_render_process_handler.unwrap()(app);
        if rph.is_null() {
            return;
        }

        log(format!("RPH found on PID {}", std::process::id()));

        if let Some(og) = (*rph).on_context_created {
            match MinHook::create_hook(og as _, on_context_created_hook as _) {
                Ok(original) => {
                    ON_CONTEXT_CREATED_OG = Some(std::mem::transmute(original));
                    log("Created on_context_created hook");
                }
                Err(e) => {
                    log(format!("Failed to hook on_context_created {e}"));
                }
            }

            if let Err(e) = MinHook::enable_hook(og as _) {
                log(format!("Couldn't enable on_context_created hook {e}"));
            }
        } else {
            log("No event set");
        }
    }
}

unsafe extern "C" fn cef_initialize_hook(
    args: *const cef_main_args_t,
    settings: *mut _cef_settings_t,
    app: *mut cef_app_t,
    sandbox: *mut c_void,
) -> c_int {
    log(format!("CEF init call on PID {}", std::process::id()));

    unsafe {
        create_context_hook(app);

        crate::callbacks::on_entrypoint(settings);

        if let Some(func) = CEF_INITIALIZE_OG {
            return func(args, settings, app, sandbox);
        }
    }

    log("Couldn't call original cef_initialize");
    0
}

unsafe extern "C" fn cef_process_hook(
    args: *const cef_main_args_t,
    app: *mut cef_app_t,
    sandbox: *mut c_void,
) -> c_int {
    log(format!("Executing process on PID {}", std::process::id()));

    unsafe {
        create_context_hook(app);

        if let Some(func) = CEF_PROCESS_OG {
            return func(args, app, sandbox);
        }
    }

    log("Couldn't call original cef process");
    0
}

unsafe extern "C" fn on_context_created_hook(
    self_: *mut cef_render_process_handler_t,
    browser: *mut cef_browser_t,
    frame: *mut cef_frame_t,
    context: *mut _cef_v8_context_t,
) {
    log(format!("Found context on PID {}", std::process::id()));
    crate::callbacks::on_context(context);

    unsafe {
        if let Some(func) = ON_CONTEXT_CREATED_OG {
            return func(self_, browser, frame, context);
        }
    }

    log("Couldn't call original on_context_created");
}
