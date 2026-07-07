use std::collections::HashMap;
use std::sync::OnceLock;
use std::process::Command;
use serde_json::json;
use serde::{Serialize, Deserialize};
use crate::knotlink_lib::{OpenSocketQuerier, KvMapExt};
use tokio::sync::Mutex;

// ── socket ID 常量 ──────────────────────────────────────────
const APP_ID: &str = "0x00000002";
const SOCKET_PLUGIN:     &str = "0x00000011";
const SOCKET_STANDALONE: &str = "0x00000012";
const SOCKET_RECIPE:     &str = "0x00000013";

// ── 全局 Querier ─────────────────────────────────────────────
static QUERIER: OnceLock<Mutex<OpenSocketQuerier>> = OnceLock::new();

pub async fn init_querier() -> Result<(), String> {
    let querier = OpenSocketQuerier::new(
        APP_ID.into(),
        SOCKET_PLUGIN.into(),
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

// ── 通用查询辅助 ──────────────────────────────────────────────

pub(crate) async fn plugin_query(payload: &HashMap<String, String>) -> Result<String, String> {
    let q = get_querier().lock().await;
    q.query_with_ids(APP_ID, SOCKET_PLUGIN, KvMapExt::serialize(payload))
        .await
        .map_err(|e| e.to_string())
}

async fn standalone_query(payload: &HashMap<String, String>) -> Result<String, String> {
    let q = get_querier().lock().await;
    q.query_with_ids(APP_ID, SOCKET_STANDALONE, KvMapExt::serialize(payload))
        .await
        .map_err(|e| e.to_string())
}

pub(crate) async fn recipe_query(payload: &HashMap<String, String>) -> Result<String, String> {
    let q = get_querier().lock().await;
    q.query_with_ids(APP_ID, SOCKET_RECIPE, KvMapExt::serialize(payload))
        .await
        .map_err(|e| e.to_string())
}

fn kv(cmd: &str, extra: &[(&str, &str)]) -> HashMap<String, String> {
    let mut m = HashMap::new();
    m.insert("cmd".into(), cmd.into());
    for (k, v) in extra {
        m.insert(k.to_string(), v.to_string());
    }
    m
}

// ── 数据结构 ──────────────────────────────────────────────────

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct NodeDetail {
    #[serde(rename = "pluginName")]
    pub plugin_name: String,
    #[serde(rename = "appId")]
    pub app_id: String,
    #[serde(default)]
    pub name: Option<String>,
    pub author: String,
    pub version: String,
    pub description: String,
    pub status: String,
    #[serde(rename = "autoStart")]
    pub auto_start: bool,
}

#[derive(Debug, Deserialize)]
struct RawNodeDetail {
    plugin_name: String,
    app_id: String,
    #[serde(default)]
    name: Option<String>,
    author: String,
    version: String,
    description: String,
    auto_start: String,
    status: String,
}
impl From<RawNodeDetail> for NodeDetail {
    fn from(raw: RawNodeDetail) -> Self {
        NodeDetail {
            plugin_name: raw.plugin_name,
            app_id: raw.app_id,
            name: raw.name,
            author: raw.author,
            version: raw.version,
            description: raw.description,
            status: raw.status,
            auto_start: raw.auto_start == "true",
        }
    }
}

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
    pub name: Option<String>,
    pub auto_start: Option<String>,
    pub description: Option<String>,
}

#[derive(Debug, Deserialize)]
struct RawPlugin {
    app_id: String,
    author: String,
    plugin_name: String,
    status: String,
    version: String,
    #[serde(default)]
    node_type: String,
    #[serde(default)]
    description: String,
    #[serde(default)]
    name: Option<String>,
    #[serde(default)]
    auto_start: Option<String>,
}

#[derive(Debug, Deserialize)]
struct RawPluginsResponse {
    plugins: Vec<RawPlugin>,
}

#[derive(Debug, Deserialize)]
struct RawStandaloneResponse {
    standalone_nodes: Vec<RawPlugin>,
}

fn raw_to_summaries(raw: Vec<RawPlugin>) -> Vec<NodeSummary> {
    raw.into_iter().map(|p| NodeSummary {
        id: p.plugin_name,
        app_id: p.app_id,
        role: String::new(),
        status: p.status,
        hot_role: String::new(),
        author: p.author,
        version: p.version,
        node_type: p.node_type,
        name: p.name,
        auto_start: p.auto_start,
        description: Some(p.description),
    }).collect()
}

// ═══════════════════════════════════════════════════════════════
// 插入式节点命令 (socketID: 0x00000011)
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn get_plugin_list() -> Result<Vec<NodeSummary>, String> {
    let resp = plugin_query(&kv("get_plugin_list", &[])).await?;
    let raw: RawPluginsResponse = serde_json::from_str(&resp)
        .map_err(|e| format!("解析插件列表失败: {}", e))?;
    Ok(raw_to_summaries(raw.plugins))
}

#[tauri::command]
pub async fn install_plugin(zip_path: String) -> Result<(), String> {
    let resp = plugin_query(&kv("install_plugin", &[
        ("zip_path", &zip_path)
    ])).await?;
    if resp == "ok" { Ok(()) } else { Err(resp) }
}

#[tauri::command]
pub async fn refresh_plugins() -> Result<Vec<NodeSummary>, String> {
    let resp = plugin_query(&kv("refresh", &[])).await?;
    let raw: RawPluginsResponse = serde_json::from_str(&resp)
        .map_err(|e| format!("解析失败: {}", e))?;
    Ok(raw_to_summaries(raw.plugins))
}

#[tauri::command]
pub async fn start_plugin(node_id: String) -> Result<(), String> {
    let resp = plugin_query(&kv("plugin_control", &[
        ("action", "start"), ("plugin_name", &node_id)
    ])).await?;
    if resp == "ok" { Ok(()) } else { Err(resp) }
}

#[tauri::command]
pub async fn stop_plugin(node_id: String) -> Result<(), String> {
    let resp = plugin_query(&kv("plugin_control", &[
        ("action", "stop"), ("plugin_name", &node_id)
    ])).await?;
    if resp == "ok" { Ok(()) } else { Err(resp) }
}

#[tauri::command]
pub async fn get_plugin_detail(node_id: String) -> Result<NodeDetail, String> {
    let resp = plugin_query(&kv("get_detail", &[
        ("plugin_name", &node_id)
    ])).await?;
    let raw: RawNodeDetail = serde_json::from_str(&resp)
        .map_err(|e| format!("解析失败: {}, 原始: {}", e, resp))?;
    Ok(raw.into())
}

#[tauri::command]
pub async fn get_plugin_funclist(node_id: String) -> Result<serde_json::Value, String> {
    let resp = plugin_query(&kv("get_funclist", &[
        ("plugin_name", &node_id)
    ])).await?;
    serde_json::from_str(&resp)
        .map_err(|e| format!("解析失败: {}", e))
}

#[tauri::command]
pub async fn set_plugin_autostart(node_id: String, auto_start: bool) -> Result<(), String> {
    let autostart = if auto_start { "true" } else { "false" };
    let resp = plugin_query(&kv("update_config", &[
        ("plugin_name", &node_id), ("autostart", autostart)
    ])).await?;
    if resp == "successful" { Ok(()) } else { Err(resp) }
}

// ═══════════════════════════════════════════════════════════════
// 独立式节点命令 (socketID: 0x00000012)
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn get_standalone_list() -> Result<Vec<NodeSummary>, String> {
    let resp = standalone_query(&kv("get_standalone_list", &[])).await?;
    let raw: RawStandaloneResponse = serde_json::from_str(&resp)
        .map_err(|e| format!("解析独立式列表失败: {}", e))?;
    Ok(raw_to_summaries(raw.standalone_nodes))
}

#[tauri::command]
pub async fn refresh_standalone() -> Result<Vec<NodeSummary>, String> {
    let resp = standalone_query(&kv("refresh", &[])).await?;
    let raw: RawStandaloneResponse = serde_json::from_str(&resp)
        .map_err(|e| format!("解析失败: {}", e))?;
    Ok(raw_to_summaries(raw.standalone_nodes))
}

#[tauri::command]
pub async fn get_standalone_detail(node_id: String) -> Result<NodeDetail, String> {
    let resp = standalone_query(&kv("get_detail", &[
        ("plugin_name", &node_id)
    ])).await?;
    let raw: RawNodeDetail = serde_json::from_str(&resp)
        .map_err(|e| format!("解析失败: {}, 原始: {}", e, resp))?;
    Ok(raw.into())
}

#[tauri::command]
pub async fn get_standalone_funclist(node_id: String) -> Result<serde_json::Value, String> {
    let resp = standalone_query(&kv("get_funclist", &[
        ("plugin_name", &node_id)
    ])).await?;
    serde_json::from_str(&resp)
        .map_err(|e| format!("解析失败: {}", e))
}

// ═══════════════════════════════════════════════════════════════
// 动态调用 (call_open_socket — 保持兼容)
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn call_open_socket(
    app_id: String,
    open_socket_id: String,
    args: HashMap<String, String>,
) -> Result<String, String> {
    let q = get_querier().lock().await;
    q.query_with_ids(&app_id, &open_socket_id, KvMapExt::serialize(&args))
        .await
        .map_err(|e| e.to_string())
}

// ═══════════════════════════════════════════════════════════════
// 保留旧命令兼容（转发到 plugin）
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn get_nodes_list() -> Result<Vec<NodeSummary>, String> {
    get_plugin_list().await
}

#[tauri::command]
pub async fn refresh_nodes() -> Result<Vec<NodeSummary>, String> {
    refresh_plugins().await
}

#[tauri::command]
pub async fn start_node(node_id: String) -> Result<(), String> {
    start_plugin(node_id).await
}

#[tauri::command]
pub async fn stop_node(node_id: String) -> Result<(), String> {
    stop_plugin(node_id).await
}

#[tauri::command]
pub async fn get_node_detail(node_id: String) -> Result<NodeDetail, String> {
    get_plugin_detail(node_id).await
}

#[tauri::command]
pub async fn get_node_manifest(node_id: String) -> Result<serde_json::Value, String> {
    get_plugin_funclist(node_id).await
}

#[tauri::command]
pub async fn set_node_autostart(node_id: String, auto_start: bool) -> Result<(), String> {
    set_plugin_autostart(node_id, auto_start).await
}

#[tauri::command]
pub async fn delete_node(plugin_name: String) -> Result<(), String> {
    // 尚未实现删除 API
    Err("delete not implemented".into())
}

#[tauri::command]
pub async fn update_node_settings(plugin_name: String, settings: String) -> Result<(), String> {
    println!("update_node_settings {} {}", plugin_name, settings);
    Ok(())
}

#[tauri::command]
pub async fn open_node_home(plugin_name: String) -> Result<(), String> {
    println!("open_node_home {}", plugin_name);
    Ok(())
}

// ═══════════════════════════════════════════════════════════════
// 系统设置
// ═══════════════════════════════════════════════════════════════

const RUN_KEY: &str = r"HKCU\Software\Microsoft\Windows\CurrentVersion\Run";
const RUN_VALUE: &str = "KnotHub";

#[tauri::command]
pub async fn get_core_autostart() -> Result<bool, String> {
    let output = Command::new("reg")
        .args(["query", RUN_KEY, "/v", RUN_VALUE])
        .output()
        .map_err(|e| e.to_string())?;
    Ok(output.status.success())
}

#[tauri::command]
pub async fn set_core_autostart(enable: bool) -> Result<(), String> {
    if enable {
        let exe = std::env::current_exe().map_err(|e| e.to_string())?;
        let dir = exe.parent().ok_or("no parent dir")?;
        let core = dir.join("KnotHubCore.exe");
        let path = core.to_string_lossy().to_string();
        Command::new("reg")
            .args(["add", RUN_KEY, "/v", RUN_VALUE, "/d", &path, "/f"])
            .output()
            .map_err(|e| e.to_string())?;
    } else {
        Command::new("reg")
            .args(["delete", RUN_KEY, "/v", RUN_VALUE, "/f"])
            .output()
            .map_err(|e| e.to_string())?;
    }
    Ok(())
}

#[tauri::command]
pub async fn get_knotlink_addr() -> Result<String, String> {
    Ok("127.0.0.1:6376".into())
}

#[tauri::command]
pub async fn open_folder(path: String) -> Result<(), String> {
    let win_path = path.replace('/', "\\");
    // explorer 打开文件夹
    std::process::Command::new("explorer")
        .arg(&win_path)
        .spawn()
        .map_err(|e| format!("{}", e))?;
    Ok(())
}

#[tauri::command]
pub async fn open_app_dir(sub: String) -> Result<(), String> {
    let exe = std::env::current_exe().map_err(|e| e.to_string())?;
    let dir = exe.parent().ok_or("no parent dir")?.join(&sub);
    // 确保目录存在，否则 explorer 会打开文档文件夹
    std::fs::create_dir_all(&dir).map_err(|e| format!("{}", e))?;
    let path = dir.to_string_lossy().replace('/', "\\");
    std::process::Command::new("explorer")
        .arg(&path)
        .spawn()
        .map_err(|e| format!("{}", e))?;
    Ok(())
}
