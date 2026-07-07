// src-tauri/src/main.rs
mod nodes;
mod recipes;
mod knotlink_lib;

// 重新导出常用类型（可选，方便命令中使用）
use knotlink_lib::{OpenSocketQuerier, SignalSender, OpenSocketResponser};
use std::net::TcpStream;
use std::time::Duration;

#[tauri::command]
fn check_service_port(addr: String) -> bool {
    TcpStream::connect_timeout(
        &addr.parse().unwrap(),
        Duration::from_secs(2)
    ).is_ok()
}

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
    // 旧命令（兼容）
    nodes::get_nodes_list,
    nodes::refresh_nodes,
    nodes::start_node,
    nodes::stop_node,
    nodes::get_node_detail,
    nodes::get_node_manifest,
    nodes::set_node_autostart,
    nodes::delete_node,
    nodes::update_node_settings,
    nodes::open_node_home,
    // 插入式节点
    nodes::get_plugin_list,
    nodes::install_plugin,
    nodes::refresh_plugins,
    nodes::start_plugin,
    nodes::stop_plugin,
    nodes::get_plugin_detail,
    nodes::get_plugin_funclist,
    nodes::set_plugin_autostart,
    // 独立式节点
    nodes::get_standalone_list,
    nodes::refresh_standalone,
    nodes::get_standalone_detail,
    nodes::get_standalone_funclist,
    // 动态调用
    nodes::call_open_socket,
    // 配方
    recipes::get_recipe_tree,
    recipes::recipe_run,
    recipes::recipe_stop,
    recipes::recipe_status,
    recipes::recipe_read,
    recipes::recipe_save,
    recipes::recipe_delete,
    recipes::recipe_import,
    recipes::recipe_create_folder,
    recipes::get_recipes_root,
    recipes::get_plugins_root,
    // 设置
    nodes::get_core_autostart,
    nodes::set_core_autostart,
    nodes::get_knotlink_addr,
    nodes::open_folder,
    nodes::open_app_dir,
    query_node,
    check_service_port,
])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}