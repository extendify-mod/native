use bindgen;
use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=./cef/Release");
    println!("cargo:rustc-link-lib=libcef");

    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_arg("-I./cef")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .allowlist_item("cef_v8*")
        .allowlist_type("cef_main_args_t")
        .allowlist_type("_cef_settings_t")
        .allowlist_type("cef_app_t")
        .allowlist_type("cef_render_process_handler_t")
        .allowlist_type("cef_browser_t")
        .allowlist_type("cef_frame_t")
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings");
}
