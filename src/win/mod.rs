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

macro_rules! define_hook {
    ($symbol:expr, $hook:expr, $original:expr) => {
        unsafe {
            match MinHook::create_hook_api("libcef.dll", $symbol, $hook as _) {
                Ok(original) => {
                    $original = Some(std::mem::transmute(original));
                    log(format!("Created {} hook", stringify!($symbol)));
                }
                Err(e) => {
                    log(format!(
                        "Couldn't create {} hook {}",
                        stringify!($symbol),
                        e
                    ));
                }
            }
        }
    };
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

fn init_hooks() {
    log("Force-loading CEF");

    let name: Vec<u16> = "libcef.dll"
        .encode_utf16()
        .chain(std::iter::once(0))
        .collect();
    unsafe { LoadLibraryW(name.as_ptr()) };

    log("Initializing hooks");

    define_hook!("cef_initialize", cef_initialize_hook, CEF_INITIALIZE_OG);
    define_hook!("cef_execute_process", cef_process_hook, CEF_PROCESS_OG);

    unsafe {
        if let Err(e) = MinHook::enable_all_hooks() {
            log(format!("Couldn't enable hooks {e}"));
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
        if let Some(func) = CEF_PROCESS_OG {
            return func(args, app, sandbox);
        }
    }

    log("Couldn't call original cef process");
    0
}
