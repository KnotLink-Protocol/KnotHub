use serde::{Serialize, Deserialize};


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

#[tauri::command]
pub async fn get_node_detail(node_id: String) -> Result<NodeDetail, String> {
    
    let mock_detail = NodeDetail {
        plugin_name: node_id.clone(),
        app_id: "0x0000A001".to_string(),
        author: "课堂助手团队".to_string(),
        version: "v1.2.0".to_string(),
        description: format!("节点 {} 的描述信息", node_id),
        status: "运行中".to_string(),
        auto_start: true,
    };
    Ok(mock_detail)
}

#[tauri::command]
pub async fn set_node_autostart(node_id: String, auto_start: bool) -> Result<(), String> {
    println!("模拟保存：节点 {} 的自启动设置为 {}", node_id, auto_start);
    Ok(())
}