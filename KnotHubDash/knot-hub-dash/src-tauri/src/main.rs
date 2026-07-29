// src-tauri/src/main.rs
// 桌面入口 — 委托到库 crate

#![windows_subsystem = "windows"]

fn main() {
    knot_hub_dash_lib::run();
}
