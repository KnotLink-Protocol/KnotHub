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

    println!("get_node_detail");

    let querier = get_querier();

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
    let querier = get_querier();
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
    pub status: String,     // 直接使用字符串 "运行中" 或 "停止"
    pub hot_role: String,
    pub author: String,
    pub version: String,
}
// ---------- 解析服务端 JSON 的中间结构 ----------
#[derive(Debug, Deserialize)]
struct RawPlugin {
    app_id: String,
    author: String,
    plugin_name: String,
    status: String,         // 字符串类型
    version: String,
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
    let querier = get_querier();   // 确保已初始化

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
            role: "".to_string(),         // 若服务端未提供，可留空或设置默认
            status: p.status,              // 直接使用 "运行中"/"停止"
            hot_role: "".to_string(),      // 同理
            author: p.author,
            version: p.version,
        }
    }).collect();

    Ok(summaries)
}

// 启动节点
#[tauri::command]
pub async fn start_node(plugin_name: String) -> Result<(), String> {
    let querier = get_querier();
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
    let querier = get_querier();
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
    let querier = get_querier();
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
    let querier = get_querier();
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
    // 先返回一个固定示例，确保功能正常
    let manifest = json!({
  "appName": "系统操作工具",
  "openSocket": {
    "SSS": {
      "appID": "0x00000015",
      "openSocketID": "0x00000011",
      "description": "",
      "args": {
        "cmd": {
          "type": "optional",
          "description": "操作",
          "options": [
            [
              "关机",
              "shutdown"
            ],
            [
              "睡眠",
              "sleep"
            ],
            [
              "锁屏",
              "lockScreen"
            ]
          ]
        }
      },
      "returns": []
    },
    "findWindowByTitle": {
      "appID": "0x00000015",
      "openSocketID": "0x00000011",
      "description": "",
      "args": {
        "cmd": {
          "type": "optional",
          "description": "",
          "options": [
            [
              "获取窗口句柄",
              "findWindowByTitle"
            ]
          ]
        },
        "title": {
          "type": "input",
          "description": "窗口标题",
          "defaultVal": "t"
        }
      },
      "returns": [
        [
          "句柄",
          "hwnd"
        ]
      ]
    },
    "setWindowState": {
      "appID": "0x00000015",
      "openSocketID": "0x00000011",
      "description": "",
      "args": {
        "cmd": {
          "type": "optional",
          "description": "",
          "options": [
            [
              "设置窗体状态",
              "setWindowState"
            ]
          ]
        },
        "hwnd": {
          "type": "input",
          "description": "句柄",
          "defaultVal": "0"
        },
        "state": {
          "type": "optional",
          "description": "状态",
          "options": [
            [
              "隐藏",
              "SW_HIDE"
            ],
            [
              "显示",
              "SW_SHOW"
            ],
            [
              "最小化",
              "SW_MINIMIZE"
            ],
            [
              "最大化",
              "SW_MAXIMIZE"
            ],
            [
              "恢复",
              "SW_RESTORE"
            ]
          ]
        }
      },
      "returns": []
    }
  },
  "signal": {}
});
    Ok(manifest)
}

#[tauri::command]
pub async fn call_open_socket(
    appId: String,
    openSocketId: String,
    args: HashMap<String, String>,
) -> Result<String, String> {
    let querier = get_querier();
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