use std::collections::HashMap;
use std::sync::OnceLock;
use serde::{Serialize, Deserialize};
use crate::knotlink_lib::{OpenSocketQuerier, KvMapExt};

// ---------- 数据结构 ----------
#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct NodeDetail {
    #[serde(rename = "pluginName")]
    pub plugin_name: String,
    #[serde(rename = "appId")]
    pub app_id: String,
    pub author: String,
    pub version: String,
    pub description: String,
    pub status: String,
    #[serde(rename = "autoStart")]
    pub auto_start: bool,
}

// 放在 nodes.rs 中
#[derive(Debug, serde::Deserialize)]
struct RawNodeDetail {
    plugin_name: String,
    app_id: String,
    author: String,
    version: String,
    description: String,
    auto_start: String,   // 服务端返回字符串 "true" / "false"
    status:String,
    // 忽略 exe_path 字段（不定义即可）
}
impl From<RawNodeDetail> for NodeDetail {
    fn from(raw: RawNodeDetail) -> Self {
        NodeDetail {
            plugin_name: raw.plugin_name,
            app_id: raw.app_id,
            author: raw.author,
            version: raw.version,
            description: raw.description,
            // 根据版本或其他字段推导状态（示例）
            status: raw.status,
            // 将字符串 "true" 转为 true，其他为 false
            auto_start: raw.auto_start == "true",
        }
    }
}
// ---------- 全局 Querier ----------
static QUERIER: OnceLock<OpenSocketQuerier> = OnceLock::new();

/// 初始化全局 Querier（在 main.rs 的 setup 中调用）
pub async fn init_querier() -> Result<(), String> {
    let querier = OpenSocketQuerier::new(
        "0x00000002".into(),
        "0x00000011".into(),
        "127.0.0.1:6376"
    )
    .await
    .map_err(|e| e.to_string())?;
    QUERIER.set(querier)
        .map_err(|_| "Querier already initialized".to_string())?;
    Ok(())
}

/// 获取 Querier 引用（内部使用）
fn get_querier() -> &'static OpenSocketQuerier {
    QUERIER.get().expect("Querier not initialized. Did you call init_querier()?")
}

// ---------- Tauri 命令 ----------
#[tauri::command]
pub async fn get_node_detail(node_id: String) -> Result<NodeDetail, String> {

    let querier = get_querier();

    let mut req = HashMap::new();
    req.insert("cmd".to_string(), "get_detail".to_string());
    req.insert("plugin_name".to_string(), "TestPlugin".to_string());
    let request = KvMapExt::serialize(&req);

    let response = querier.query_l(request).await.map_err(|e| e.to_string())?;

    let raw: RawNodeDetail = serde_json::from_str(&response)
        .map_err(|e| format!("解析响应失败: {}, 原始: {}", e, response))?;

    // let mock_detail = NodeDetail {
    //     plugin_name: node_id.clone(),
    //     app_id: "0x0000A001".to_string(),
    //     author: "课堂助手团队".to_string(),
    //     version: "v1.2.0".to_string(),
    //     description: format!("节点 {} 的描述信息", node_id),
    //     status: "运行中".to_string(),
    //     auto_start: true,
    // };
    Ok(raw.into())
}

#[tauri::command]
pub async fn set_node_autostart(node_id: String, auto_start: bool) -> Result<(), String> {
    println!("模拟保存：节点 {} 的自启动设置为 {}", node_id, auto_start);
    Ok(())
}