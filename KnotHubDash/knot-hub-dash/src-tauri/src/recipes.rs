use serde::{Serialize, Deserialize};
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

// ── 辅助：走 recipe_query → socketID 0x00000013 ────────────

fn kv(cmd: &str, extra: &[(&str, &str)]) -> HashMap<String, String> {
    let mut m = HashMap::new();
    m.insert("cmd".into(), cmd.into());
    for (k, v) in extra {
        m.insert(k.to_string(), v.to_string());
    }
    m
}

// ═══════════════════════════════════════════════════════════════
// Tauri 命令 — socketID: 0x00000013
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn get_recipe_tree() -> Result<RecipeTreeNode, String> {
    let resp = crate::nodes::recipe_query(&kv("get_recipe_tree", &[])).await?;
    serde_json::from_str(&resp)
        .map_err(|e| format!("解析配方树失败: {}, 原始: {}", e, resp))
}

#[tauri::command]
pub async fn recipe_run(file_path: String) -> Result<String, String> {
    crate::nodes::recipe_query(&kv("recipe_run", &[("file_path", &file_path)])).await
}

#[tauri::command]
pub async fn recipe_stop(file_path: String) -> Result<String, String> {
    crate::nodes::recipe_query(&kv("recipe_stop", &[("file_path", &file_path)])).await
}

#[tauri::command]
pub async fn recipe_status(file_path: String) -> Result<String, String> {
    crate::nodes::recipe_query(&kv("recipe_status", &[("file_path", &file_path)])).await
}

#[tauri::command]
pub async fn recipe_read(file_path: String) -> Result<String, String> {
    crate::nodes::recipe_query(&kv("recipe_read", &[("file_path", &file_path)])).await
}

#[tauri::command]
pub async fn recipe_save(file_path: String, content: String) -> Result<String, String> {
    crate::nodes::recipe_query(&kv("recipe_save", &[
        ("file_path", &file_path), ("content", &content)
    ])).await
}

#[tauri::command]
pub async fn recipe_delete(file_path: String) -> Result<String, String> {
    crate::nodes::recipe_query(&kv("recipe_delete", &[("file_path", &file_path)])).await
}

#[tauri::command]
pub async fn recipe_import(
    source_path: String,
    target_dir: String,
    overwrite: bool,
) -> Result<String, String> {
    let ow = if overwrite { "true" } else { "false" };
    crate::nodes::recipe_query(&kv("import_recipe", &[
        ("source_path", &source_path),
        ("target_dir", &target_dir),
        ("overwrite", ow),
    ])).await
}

#[tauri::command]
pub async fn recipe_create_folder(path: String) -> Result<String, String> {
    crate::nodes::recipe_query(&kv("create_folder", &[("path", &path)])).await
}
