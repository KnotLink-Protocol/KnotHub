use serde::{Serialize, Deserialize};
use crate::knotlink_lib::KvMapExt;
use std::collections::HashMap;

// ── 数据 ─────────────────────────────────────────────────────

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct RecipeTreeNode {
    pub id: String,
    pub name: String,
    #[serde(rename = "type")]
    pub node_type: String,
    #[serde(default)]
    pub children: Vec<RecipeTreeNode>,
    #[serde(default)]
    pub status: String,
}

// ── 辅助 ─────────────────────────────────────────────────────

fn send_cmd(cmd: &str, extra: &[(&str, &str)]) -> HashMap<String, String> {
    let mut m: HashMap<String, String> = HashMap::new();
    m.insert("cmd".into(), cmd.into());
    for (k, v) in extra {
        m.insert(k.to_string(), v.to_string());
    }
    m
}

async fn recipe_cmd(cmd: &str, extra: &[(&str, &str)]) -> Result<String, String> {
    let querier = crate::nodes::get_querier().lock().await;
    let payload = send_cmd(cmd, extra);
    let request = KvMapExt::serialize(&payload);
    querier.query_l(request).await.map_err(|e| e.to_string())
}

// ── Tauri 命令 ───────────────────────────────────────────────

#[tauri::command]
pub async fn get_recipe_tree() -> Result<RecipeTreeNode, String> {
    let response = recipe_cmd("get_recipe_tree", &[]).await?;
    serde_json::from_str(&response)
        .map_err(|e| format!("解析配方树失败: {}, 原始: {}", e, response))
}

#[tauri::command]
pub async fn recipe_run(file_path: String) -> Result<String, String> {
    recipe_cmd("recipe_run", &[("file_path", &file_path)]).await
}

#[tauri::command]
pub async fn recipe_stop(file_path: String) -> Result<String, String> {
    recipe_cmd("recipe_stop", &[("file_path", &file_path)]).await
}

#[tauri::command]
pub async fn recipe_status(file_path: String) -> Result<String, String> {
    recipe_cmd("recipe_status", &[("file_path", &file_path)]).await
}

#[tauri::command]
pub async fn recipe_read(file_path: String) -> Result<String, String> {
    recipe_cmd("recipe_read", &[("file_path", &file_path)]).await
}

#[tauri::command]
pub async fn recipe_save(file_path: String, content: String) -> Result<String, String> {
    recipe_cmd("recipe_save", &[("file_path", &file_path), ("content", &content)]).await
}

#[tauri::command]
pub async fn recipe_delete(file_path: String) -> Result<String, String> {
    recipe_cmd("recipe_delete", &[("file_path", &file_path)]).await
}
