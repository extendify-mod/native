use std::fmt::Display;
use std::fs::OpenOptions;
use std::io::Write;

mod callbacks;
mod cef;

#[cfg(target_os = "windows")]
mod win;

pub fn log<T: Display>(msg: T) {
    if let Ok(mut file) = OpenOptions::new()
        .create(true)
        .append(true)
        .open("spotify_hook.log")
    {
        writeln!(file, "{msg}").ok();
    }
}

pub fn clear_log() {
    if let Ok(mut file) = OpenOptions::new()
        .create(true)
        .append(false)
        .open("spotify_hook.log")
    {
        writeln!(file, "").ok();
    }
}
