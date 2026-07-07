use std::collections::HashMap;
use std::sync::OnceLock;
use serde_json::json;
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
use tokio::sync::Mutex;   // 异步锁

static QUERIER: OnceLock<Mutex<OpenSocketQuerier>> = OnceLock::new();

pub async fn init_querier() -> Result<(), String> {
    let querier = OpenSocketQuerier::new(
        "0x00000002".into(),
        "0x00000011".into(),
        "127.0.0.1:6376"
    )
    .await
    .map_err(|e| e.to_string())?;
    QUERIER.set(Mutex::new(querier))
        .map_err(|_| "Querier already initialized".to_string())?;
    Ok(())
}

pub(crate) fn get_querier() -> &'static Mutex<OpenSocketQuerier> {
    QUERIER.get().expect("Querier not initialized")
}

// ---------- Tauri 命令 ----------
#[tauri::command]
pub async fn get_node_detail(node_id: String) -> Result<NodeDetail, String> {

    println!("get_node_detail");

    let querier = get_querier().lock().await;

    let mut req = HashMap::new();
    req.insert("cmd".to_string(), "get_detail".to_string());
    req.insert("plugin_name".to_string(), node_id);
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
    let querier = get_querier().lock().await;
    let request = format!("cmd=update_config;plugin_name={};autostart={}", node_id,auto_start);
    let response = querier.query_l(request).await.map_err(|e| e.to_string())?;
    if response == "successful" {
        Ok(())
    } else {
        Err(format!("设置失败: {}", response))
    }
}


// ---------- 前端数据结构 ----------
#[derive(Serialize)]
pub struct NodeSummary {
    pub id: String,
    pub app_id: String,
    pub role: String,
    pub status: String,
    pub hot_role: String,
    pub author: String,
    pub version: String,
    pub node_type: String,
}
// ---------- 解析服务端 JSON 的中间结构 ----------
#[derive(Debug, Deserialize)]
struct RawPlugin {
    app_id: String,
    author: String,
    plugin_name: String,
    status: String,
    version: String,
    #[serde(default)]
    node_type: String,
}

#[derive(Debug, Deserialize)]
struct RawPluginsResponse {
    plugins: Vec<RawPlugin>,
}

// ---------- 命令：获取节点列表 ----------
#[tauri::command]
pub async fn get_nodes_list() -> Result<Vec<NodeSummary>, String> {

    println!("get_nodes_list");

    // 假设你已有全局 Querier（通过 OnceLock 管理）
    let querier = get_querier().lock().await;   // 确保已初始化

    // 发送请求（协议需与服务端一致）
    let request = "cmd=get_plugin_list".to_string();
    let response = querier.query_l(request)
        .await
        .map_err(|e| e.to_string())?;

    // 解析 JSON
    let raw_response: RawPluginsResponse = serde_json::from_str(&response)
        .map_err(|e| format!("解析插件列表失败: {}", e))?;

    // 转换为 NodeSummary
    let summaries = raw_response.plugins.into_iter().map(|p| {
        NodeSummary {
            id: p.plugin_name,
            app_id: p.app_id,
            role: String::new(),
            status: p.status,
            hot_role: String::new(),
            author: p.author,
            version: p.version,
            node_type: p.node_type,
        }
    }).collect();

    Ok(summaries)
}

// 启动节点
#[tauri::command]
pub async fn start_node(plugin_name: String) -> Result<(), String> {
    let querier = get_querier().lock().await;
    let request = format!("cmd=plugin_control;action=start;plugin_name={}", plugin_name);
    let response = querier.query_l(request).await.map_err(|e| e.to_string())?;
    if response == "ok" {
        Ok(())
    } else {
        Err(format!("启动失败: {}", response))
    }
}

// 停止节点
#[tauri::command]
pub async fn stop_node(plugin_name: String) -> Result<(), String> {
    let querier = get_querier().lock().await;
    let request = format!("cmd=plugin_control;action=stop;plugin_name={}", plugin_name);
    let response = querier.query_l(request).await.map_err(|e| e.to_string())?;
    if response == "ok" {
        Ok(())
    } else {
        Err(format!("停止失败: {}", response))
    }
}

// 删除节点
#[tauri::command]
pub async fn delete_node(plugin_name: String) -> Result<(), String> {
    let querier = get_querier().lock().await;
    let request = format!("cmd=delete;plugin_name={}", plugin_name);
    let response = querier.query_l(request).await.map_err(|e| e.to_string())?;
    if response == "ok" {
        Ok(())
    } else {
        Err(format!("删除失败: {}", response))
    }
}

// 更新节点设置（示例：传递 JSON 或键值对）
#[tauri::command]
pub async fn update_node_settings(plugin_name: String, settings: String) -> Result<(), String> {
    let querier = get_querier().lock().await;
    // settings 可以是 JSON 字符串，例如 {"role":"主控"}
    let request = format!("cmd=update_settings;plugin_name={};settings={}", plugin_name, settings);
    let response = querier.query_l(request).await.map_err(|e| e.to_string())?;
    if response == "ok" {
        Ok(())
    } else {
        Err(format!("更新设置失败: {}", response))
    }
}

// 获取节点主页（可能返回 URL 或直接打开）
#[tauri::command]
pub async fn open_node_home(plugin_name: String) -> Result<(), String> {
    // 简单实现：打印日志，或调用系统浏览器
    println!("打开节点 {} 的主页", plugin_name);
    // 如果服务端有主页地址，可以查询后打开
    Ok(())
}
// 放在 nodes.rs 末尾
#[tauri::command]
pub async fn get_node_manifest(nodeId: String) -> Result<serde_json::Value, String> {
    println!("🔍 get_node_manifest called with nodeId: {}", nodeId);
    
    // 获取全局 Querier（单例，复用连接）
    let querier = get_querier().lock().await;  // 注意：需要加锁，因为 get_querier() 返回的是 &'static OpenSocketQuerier（未加锁）
    // 但你的全局 QUERIER 目前是 OnceLock<OpenSocketQuerier>，不是 Mutex，所以可以直接用。
    // 如果以后改成带锁的，记得 .lock().unwrap()

    // 构造 KL 请求（根据你的实际协议调整）
    let mut req = HashMap::new();
    req.insert("cmd".to_string(), "get_funclist".to_string());
    req.insert("plugin_name".to_string(), nodeId);
    let request = KvMapExt::serialize(&req);  // 例如 "cmd=get_manifest;plugin_name=xxx"

    // 发送查询，等待响应
    let response = querier.query_l(request)
        .await
        .map_err(|e| format!("查询功能清单失败: {}", e))?;

    // 解析响应（假设服务端返回 JSON 格式）
    let manifest: serde_json::Value = serde_json::from_str(&response)
        .map_err(|e| format!("解析功能清单 JSON 失败: {}, 原始: {}", e, response))?;

    // 可选：验证返回的结构是否符合预期（appName, openSocket, signal 等）
    // 如果不符合，可以返回错误或默认空清单
    if !manifest.is_object() {
        return Err("服务端返回的不是有效的 JSON 对象".to_string());
    }

    Ok(manifest)
}

#[tauri::command]
pub async fn call_open_socket(
    appId: String,
    openSocketId: String,
    args: HashMap<String, String>,
) -> Result<String, String> {
    let querier = get_querier().lock().await;
    let mut req = HashMap::new();
    // 注意：app_id 和 open_socket_id 不再需要放在请求体里（由前缀携带）
    // 但仍可以保留作为请求参数，视协议而定
    for (k, v) in args {
        req.insert(k, v);
    }
    let request = KvMapExt::serialize(&req);
    
    // 使用 query_with_ids 传递临时 ID
    let response = querier
        .query_with_ids(&appId, &openSocketId, request)
        .await
        .map_err(|e| e.to_string())?;
    Ok(response)
}

#[tauri::command]
pub async fn refresh_nodes() -> Result<Vec<NodeSummary>, String> {
    let querier = get_querier().lock().await;
    let mut req: HashMap<String, String> = HashMap::new();
    req.insert("cmd".into(), "refresh".into());
    let request = KvMapExt::serialize(&req);
    let response = querier.query_l(request).await.map_err(|e| e.to_string())?;

    let raw_response: RawPluginsResponse = serde_json::from_str(&response)
        .map_err(|e| format!("解析插件列表失败: {}", e))?;

    let summaries = raw_response.plugins.into_iter().map(|p| {
        NodeSummary {
            id: p.plugin_name,
            app_id: p.app_id,
            role: String::new(),
            status: p.status,
            hot_role: String::new(),
            author: p.author,
            version: p.version,
            node_type: p.node_type,
        }
    }).collect();

    Ok(summaries)
}