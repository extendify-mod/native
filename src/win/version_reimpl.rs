use crate::win::log;
use core::ffi::c_void;
use windows_sys::Win32::Foundation::FARPROC;
use windows_sys::Win32::System::LibraryLoader::{GetProcAddress, LoadLibraryW};
use windows_sys::core::{BOOL, PCWSTR};

type GetFileVersionInfoSizeFn = unsafe extern "system" fn(PCWSTR, *mut u32) -> u32;
type GetFileVersionInfoFn = unsafe extern "system" fn(PCWSTR, u32, u32, *mut c_void) -> BOOL;
type VerQueryValueFn =
    unsafe extern "system" fn(*const c_void, PCWSTR, *mut *mut c_void, *mut u32) -> BOOL;

fn get_function(name: &[u8]) -> FARPROC {
    let dll_name: Vec<u16> = "C:\\Windows\\System32\\version.dll"
        .encode_utf16()
        .chain(std::iter::once(0))
        .collect();

    unsafe {
        let lib = LoadLibraryW(dll_name.as_ptr());
        if lib == std::ptr::null_mut() {
            log("Failed to load DLL");
            return None;
        }

        GetProcAddress(lib, name.as_ptr())
    }
}

#[unsafe(no_mangle)]
pub extern "system" fn GetFileVersionInfoSizeW(filename: PCWSTR, handle: *mut u32) -> u32 {
    unsafe {
        if let Some(func) = get_function(b"GetFileVersionInfoSizeW\0") {
            let callable: GetFileVersionInfoSizeFn = std::mem::transmute(func);
            return callable(filename, handle);
        }

        log("DLL not loaded");
        0
    }
}

#[unsafe(no_mangle)]
pub extern "system" fn GetFileVersionInfoW(
    filename: PCWSTR,
    handle: u32,
    len: u32,
    data: *mut c_void,
) -> BOOL {
    unsafe {
        if let Some(func) = get_function(b"GetFileVersionInfoW\0") {
            let callable: GetFileVersionInfoFn = std::mem::transmute(func);
            return callable(filename, handle, len, data);
        }

        log("DLL not loaded");
        0
    }
}

#[unsafe(no_mangle)]
pub extern "system" fn VerQueryValueW(
    pblock: *const c_void,
    sub_block: PCWSTR,
    buffer: *mut *mut c_void,
    len: *mut u32,
) -> BOOL {
    unsafe {
        if let Some(func) = get_function(b"VerQueryValueW\0") {
            let callable: VerQueryValueFn = std::mem::transmute(func);
            return callable(pblock, sub_block, buffer, len);
        }

        log("DLL not loaded");
        0
    }
}
