// src-tauri/src/main.rs
mod nodes;  // 导入节点模块（需在 src-tauri/src/nodes.rs 中实现）
// 声明 knotlink_lib 模块（模块文件位于 src/knotlink_lib/）
mod knotlink_lib;

// 重新导出常用类型（可选，方便命令中使用）
use knotlink_lib::{OpenSocketQuerier, SignalSender, OpenSocketResponser};

#[tauri::command]
async fn query_node(plugin_name: String) -> Result<String, String> {
    let querier = knotlink_lib::OpenSocketQuerier::new(
        "0x00000002".into(),
        "0x00000011".into(),
        "127.0.0.1:6376"
    )
    .await
    .map_err(|e| e.to_string())?;

    let result = querier.query_l(plugin_name).await.map_err(|e| e.to_string())?;
    Ok(result)
}

use tauri::{
    menu::{Menu, MenuItem},
    tray::{TrayIconBuilder, TrayIconEvent},
    Manager, Runtime, WindowEvent,
};

pub fn create_tray<R: Runtime>(app: &tauri::AppHandle<R>) -> tauri::Result<()> {
    // 创建菜单项
    let show_i = MenuItem::with_id(app, "show", "显示主窗口", true, None::<&str>)?;
    let quit_i = MenuItem::with_id(app, "quit", "退出", true, None::<&str>)?;
    let menu = Menu::with_items(app, &[&show_i, &quit_i])?;

    // 构建并生成托盘图标
    TrayIconBuilder::with_id("tray")
        .icon(app.default_window_icon().unwrap().clone())
        .menu(&menu)
        .menu_on_left_click(false) // 禁用左键弹出菜单
        .on_tray_icon_event(|tray, event| {
            if let TrayIconEvent::Click { .. } = event {
                // 左键单击托盘图标时显示/隐藏窗口
                let app_handle = tray.app_handle();
                if let Some(window) = app_handle.get_webview_window("main") {
                    if window.is_visible().unwrap_or(false) {
                        let _ = window.hide();
                    } else {
                        let _ = window.show();
                        let _ = window.set_focus();
                    }
                }
            }
        })
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
        .build(app)?;
    Ok(())
}

fn main() {
    tauri::Builder::default()
        .setup(|app| {
            // 创建托盘
            create_tray(app.handle())?;

            // 拦截窗口关闭事件：隐藏窗口而非退出
            let window = app.get_webview_window("main").unwrap();
            let window_clone = window.clone();
            window.on_window_event(move |event| {
                if let WindowEvent::CloseRequested { .. } = event {
                    let _ = window_clone.hide();
                }
            });

            tauri::async_runtime::block_on(nodes::init_querier())
                .expect("Failed to init querier");

            print!("1123");

            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            nodes::get_node_detail,        // 注册命令
            nodes::set_node_autostart,     // 注册命令
            query_node,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}