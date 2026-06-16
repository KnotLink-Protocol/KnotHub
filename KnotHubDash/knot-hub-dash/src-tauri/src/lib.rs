// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/

pub mod knotlink_lib;   // 声明外部模块

// 重新导出常用的类型，方便命令中使用
pub use knotlink_lib::{OpenSocketQuerier, SignalSender, OpenSocketResponser};

#[tauri::command]
async fn query_node(plugin_name: String) -> Result<String, String> {
    let querier = knotlink_lib::OpenSocketQuerier::new(
        "0x00000002".into(),
        "0x00000011".into(),
        "127.0.0.1:6376"
    ).await.map_err(|e| e.to_string())?;

    let result = querier.query_l(plugin_name).await.map_err(|e| e.to_string())?;
    Ok(result)
}

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    // 合并命令注册
tauri::Builder::default()
    .plugin(tauri_plugin_opener::init())
    .invoke_handler(tauri::generate_handler![greet, query_node])  // 关键修改
    .run(tauri::generate_context!())
    .expect("error while running tauri application");
}

