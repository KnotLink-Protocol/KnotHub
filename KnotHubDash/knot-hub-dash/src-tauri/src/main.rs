// src-tauri/src/main.rs
use tauri::{
    menu::{Menu, MenuItem},
    tray::{TrayIconBuilder, TrayIconEvent},
    Manager, Runtime,
};

pub fn create_tray<R: Runtime>(app: &tauri::AppHandle<R>) -> tauri::Result<()> {
    // 创建菜单项
    let show_i = MenuItem::with_id(app, "show", "显示主窗口", true, None::<&str>)?;
    let quit_i = MenuItem::with_id(app, "quit", "退出", true, None::<&str>)?;
    let menu = Menu::with_items(app, &[&show_i, &quit_i])?;

    // 构建并生成托盘图标
    let _ = TrayIconBuilder::with_id("tray")
        .icon(app.default_window_icon().unwrap().clone()) // 使用应用图标
        .menu(&menu)
        .menu_on_left_click(false) // 禁用左键弹出菜单
        .on_menu_event(move |app, event| match event.id.as_ref() {
            "show" => {
                if let Some(window) = app.get_webview_window("main") {
                    let _ = window.show();
                    let _ = window.set_focus();
                }
            }
            "quit" => {
                app.exit(0);
            }
            _ => {}
        })
        .build(app);

    Ok(())
}

fn main() {
    tauri::Builder::default()
        .setup(|app| {
            create_tray(app.handle())?;
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("运行应用时出错");
}