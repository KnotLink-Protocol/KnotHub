use serde::{Deserialize, Serialize};
use tauri::ipc::Channel;
use futures::StreamExt;
use std::path::{Path, PathBuf};
use crate::knotlink_lib::{OpenSocketQuerier, SignalSubscriber};
use tokio::time::{self, Duration};

const MD_APPID: &str = "com.knotlink.multidownload";
const MD_SOCKET: &str = "download";

// ── 数据结构 ──────────────────────────────────────────────────

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct StorePlugin {
    pub id: String,
    pub name: String,
    #[serde(rename = "type")]
    pub node_type: String,
    #[serde(rename = "typeLabel")]
    pub type_label: String,
    #[serde(rename = "typeIcon")]
    pub type_icon: String,
    pub dir: String,
    pub author: String,
    pub version: String,
    pub desc: Option<String>,
    #[serde(rename = "appId")]
    pub app_id: String,
    #[serde(rename = "downloadUrl")]
    pub download_url: String,
    pub logo: Option<String>,
    #[serde(rename = "appName")]
    pub app_name: Option<String>,
    #[serde(rename = "specVersion")]
    pub spec_version: Option<String>,
    #[serde(rename = "manifestVersion")]
    pub manifest_version: Option<String>,
    #[serde(rename = "socketsCount")]
    pub sockets_count: u32,
    #[serde(rename = "signalsCount")]
    pub signals_count: u32,
}

// ═══════════════════════════════════════════════════════════════
// 命令：拉取插件市场索引
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn fetch_store_index(url: String) -> Result<Vec<StorePlugin>, String> {
    let client = reqwest::Client::builder()
        .timeout(std::time::Duration::from_secs(10))
        .build()
        .map_err(|e| format!("创建 HTTP 客户端失败: {}", e))?;

    let resp = client
        .get(&url)
        .send()
        .await
        .map_err(|e| format!("网络请求失败: {}", e))?;

    if !resp.status().is_success() {
        return Err(format!("HTTP {}", resp.status()));
    }

    let plugins: Vec<StorePlugin> = resp
        .json()
        .await
        .map_err(|e| format!("解析 JSON 失败: {}", e))?;

    Ok(plugins)
}

// ═══════════════════════════════════════════════════════════════
// 工具：GitHub releases/latest → 直接下载链接
// ═══════════════════════════════════════════════════════════════

async fn resolve_download_url(url: &str) -> Result<String, String> {
    // 检测 GitHub releases/latest 页面链接，自动转 API 查询
    if !(url.contains("github.com/") && url.ends_with("/releases/latest")) {
        return Ok(url.to_string());
    }

    let parts: Vec<&str> = url.split('/').collect();
    if parts.len() < 5 {
        return Ok(url.to_string());
    }
    let owner = parts[3];
    let repo  = parts[4];

    let client = reqwest::Client::builder()
        .timeout(std::time::Duration::from_secs(15))
        .build()
        .map_err(|e| format!("创建 HTTP 客户端失败: {}", e))?;

    // 尝试 1：调 /releases/latest（正式 release）
    let api_latest = format!(
        "https://api.github.com/repos/{}/{}/releases/latest", owner, repo);
    let resp = client
        .get(&api_latest)
        .header("User-Agent", "KnotHub-Dash")
        .header("Accept", "application/vnd.github+json")
        .send()
        .await;

    let assets: Vec<serde_json::Value> = match resp {
        Ok(r) if r.status().is_success() => {
            let json: serde_json::Value = r.json().await.map_err(|e| format!("解析失败: {}", e))?;
            json["assets"].as_array().cloned().unwrap_or_default()
        }
        _ => {
            // 尝试 2：调 /releases（列出所有 release，包括 pre-release）
            let api_all = format!(
                "https://api.github.com/repos/{}/{}/releases", owner, repo);
            let r2 = client
                .get(&api_all)
                .header("User-Agent", "KnotHub-Dash")
                .header("Accept", "application/vnd.github+json")
                .send()
                .await
                .map_err(|e| format!("GitHub API 请求失败: {}", e))?;

            if !r2.status().is_success() {
                return Err(format!("GitHub API HTTP {} — 仓库不存在或没有 release", r2.status()));
            }

            let releases: Vec<serde_json::Value> = r2.json().await
                .map_err(|e| format!("解析失败: {}", e))?;
            // 取第一个 release 的 assets
            releases.first()
                .and_then(|r| r["assets"].as_array())
                .cloned()
                .unwrap_or_default()
        }
    };

    if assets.is_empty() {
        return Err("该 release 没有附加文件（zip/exe）".into());
    }

    // 优先 .zip，否则 .exe，否则取第一个 asset
    for asset in &assets {
        let name = asset["name"].as_str().unwrap_or("");
        if name.ends_with(".zip") || name.ends_with(".exe") {
            return Ok(asset["browser_download_url"]
                .as_str()
                .unwrap_or(url)
                .to_string());
        }
    }

    Ok(assets[0]["browser_download_url"]
        .as_str()
        .unwrap_or(url)
        .to_string())
}

// ═══════════════════════════════════════════════════════════════
// 命令：下载 zip 并安装插件
// ═══════════════════════════════════════════════════════════════

#[derive(Clone, serde::Serialize)]
pub struct DownloadProgress {
    pub downloaded: u64,
    pub total: Option<u64>,
}

/// 用 reqwest 流式下载。check_zip 为 true 时验证 PK 头（插件 zip），false 时跳过（配方 .py/.kln）
async fn reqwest_download(
    url: &str,
    dest_dir: &Path,
    dest_name: &str,
    on_progress: Channel<DownloadProgress>,
    check_zip: bool,
) -> Result<PathBuf, String> {
    let client = reqwest::Client::builder()
        .timeout(std::time::Duration::from_secs(120))
        .build()
        .map_err(|e| format!("创建 HTTP 客户端失败: {}", e))?;

    let response = client.get(url).send().await
        .map_err(|e| format!("网络请求失败，GitHub 可能无法访问: {}", e))?;

    if !response.status().is_success() {
        return Err(format!("HTTP {}", response.status()));
    }

    let ct = response.headers()
        .get(reqwest::header::CONTENT_TYPE)
        .and_then(|v| v.to_str().ok())
        .unwrap_or("");
    if ct.contains("text/html") {
        return Err(format!("镜像返回了网页而非文件（Content-Type: {}），可能镜像已失效", ct));
    }

    let total = response.content_length();
    let mut bytes = Vec::new();
    let mut stream = response.bytes_stream();

    while let Some(chunk) = stream.next().await {
        let chunk = chunk.map_err(|e| format!("下载中断: {}", e))?;
        bytes.extend_from_slice(&chunk);
        let _ = on_progress.send(DownloadProgress {
            downloaded: bytes.len() as u64,
            total,
        });
    }

    if bytes.is_empty() {
        return Err("下载的文件为空".into());
    }

    if check_zip && (bytes.len() < 4 || &bytes[0..4] != b"PK\x03\x04") {
        let dump = std::env::temp_dir().join(format!("knothub_bad_{}.bin", std::process::id()));
        let _ = std::fs::write(&dump, &bytes);
        return Err(format!(
            "下载的文件不是有效 zip（可能镜像返回了错误页面）。\n已保存至: {}",
            dump.display()
        ));
    }

    let out_path = dest_dir.join(dest_name);
    std::fs::write(&out_path, &bytes)
        .map_err(|e| format!("写入临时文件失败: {}", e))?;

    Ok(out_path)
}

/// 通过 MultiDownload KL 节点下载
async fn download_via_md(
    url: &str,
    dest_dir: &Path,
    dest_name: &str,
    on_progress: Channel<DownloadProgress>,
) -> Result<PathBuf, String> {
    let req_id = uuid::Uuid::new_v4().to_string();
    let dest_path = dest_dir.join(dest_name);
    let dest_str = dest_path.to_string_lossy().to_string();

    // 订阅信号
    let mut sub_progress = SignalSubscriber::new(
        MD_APPID.to_string(), "progress".to_string()
    ).await.map_err(|e| format!("订阅 progress 失败: {}", e))?;

    let mut sub_completed = SignalSubscriber::new(
        MD_APPID.to_string(), "completed".to_string()
    ).await.map_err(|e| format!("订阅 completed 失败: {}", e))?;

    let mut sub_failed = SignalSubscriber::new(
        MD_APPID.to_string(), "failed".to_string()
    ).await.map_err(|e| format!("订阅 failed 失败: {}", e))?;

    // 发送下载请求
    let querier = OpenSocketQuerier::new(
        "0x00000002".to_string(), "md-check".to_string()
    ).await.map_err(|e| format!("连接 MultiDownload 失败: {}", e))?;

    let payload = format!(
        "cmd=start;url={};dest={};reqID={};threads=8",
        url, dest_str, req_id
    );
    let resp = querier.query_l(
        payload, MD_APPID, MD_SOCKET,
        Some(Duration::from_secs(5)),
    ).await.map_err(|e| format!("请求 MultiDownload 失败: {}", e))?;

    if resp.trim() != "OK" {
        return Err(format!("MultiDownload 拒绝请求: {}", resp));
    }
    drop(querier);

    // 等待完成信号
    let timeout = Duration::from_secs(3600);
    let deadline = time::sleep(timeout);
    tokio::pin!(deadline);

    loop {
        tokio::select! {
            msg = sub_progress.rx.recv() => {
                if let Some(data) = msg {
                    if data.contains(&format!("reqID={}", req_id)) {
                        // 解析 percent
                        if let Some(pct) = parse_kv(&data, "percent") {
                            let pct: u64 = pct.parse().unwrap_or(0);
                            let _ = on_progress.send(DownloadProgress {
                                downloaded: pct,
                                total: Some(100),
                            });
                        }
                    }
                }
            }
            msg = sub_completed.rx.recv() => {
                if let Some(data) = msg {
                    if data.contains(&format!("reqID={}", req_id)) {
                        if dest_path.exists() {
                            return Ok(dest_path);
                        }
                        return Err("下载完成但文件不存在".to_string());
                    }
                }
            }
            msg = sub_failed.rx.recv() => {
                if let Some(data) = msg {
                    if data.contains(&format!("reqID={}", req_id)) {
                        let err = parse_kv(&data, "error").unwrap_or("未知错误".to_string());
                        return Err(format!("MultiDownload 下载失败: {}", err));
                    }
                }
            }
            _ = &mut deadline => {
                return Err("下载超时".to_string());
            }
        }
    }
}

/// 从 KLKVMap 字符串中解析值
fn parse_kv(data: &str, key: &str) -> Option<String> {
    for pair in data.split(';') {
        if let Some((k, v)) = pair.split_once('=') {
            if k == key { return Some(v.to_string()); }
        }
    }
    None
}

#[tauri::command]
pub async fn download_and_install(
    url: String,
    mirror_url: Option<String>,
    on_progress: Channel<DownloadProgress>,
    use_md: Option<bool>,
) -> Result<(), String> {
    // 1. 解析下载链接（GitHub API 直连，不走镜像）
    let real_url = resolve_download_url(&url).await?;

    // 1.5 如果指定了镜像，拼到真实下载 URL 前面
    let download_url = match &mirror_url {
        Some(prefix) if !prefix.is_empty() => format!("{}{}", prefix, real_url),
        _ => real_url,
    };

    let tmp_dir = std::env::temp_dir();
    let tmp_name = format!("knothub_dl_{}.zip", std::process::id());

    // 2. 尝试 MultiDownload（如果启用且在线）
    if use_md.unwrap_or(false) {
        match download_via_md(&download_url, &tmp_dir, &tmp_name, on_progress.clone()).await {
            Ok(path) => {
                let tmp_str = path.to_string_lossy().to_string();
                let result = crate::nodes::install_plugin(tmp_str).await;
                let _ = std::fs::remove_file(&path);
                return result;
            }
            Err(e) => eprintln!("[KnotHub] MultiDownload 失败: {}, 回退 reqwest", e),
        }
    }

    // 3. reqwest 下载
    let tmp_path = reqwest_download(&download_url, &tmp_dir, &tmp_name, on_progress.clone(), true).await?;

    // 3. 安装
    let tmp_str = tmp_path.to_string_lossy().to_string();
    let result = crate::nodes::install_plugin(tmp_str).await;

    // 4. 清理
    let _ = std::fs::remove_file(&tmp_path);

    result
}

// ═══════════════════════════════════════════════════════════════
// 命令：HTTP GET 代理（前端跨域兜底）
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn http_get_text(url: String) -> Result<String, String> {
    let client = reqwest::Client::builder()
        .timeout(std::time::Duration::from_secs(10))
        .build()
        .map_err(|e| format!("创建 HTTP 客户端失败: {}", e))?;

    let resp = client
        .get(&url)
        .send()
        .await
        .map_err(|e| format!("网络请求失败: {}", e))?;

    if !resp.status().is_success() {
        return Err(format!("HTTP {}", resp.status()));
    }

    resp.text()
        .await
        .map_err(|e| format!("读取响应失败: {}", e))
}

// ═══════════════════════════════════════════════════════════════
// 命令：下载配方文件并导入
// ═══════════════════════════════════════════════════════════════

#[tauri::command]
pub async fn download_and_import_recipe(
    url: String,
    on_progress: Channel<DownloadProgress>,
) -> Result<(), String> {
    let tmp_dir = std::env::temp_dir();
    // 从 URL 提取文件名
    let file_name = url.rsplit('/').next().unwrap_or("recipe.py");
    let tmp_name = format!("knothub_recipe_{}", file_name);

    // 下载
    eprintln!("[KnotHub] 下载配方: {}", url);
    let tmp_path = reqwest_download(&url, &tmp_dir, &tmp_name, on_progress.clone(), false).await?;

    // 调 C++ RecipeManager 导入
    let tmp_str = tmp_path.to_string_lossy().to_string();
    let result = crate::nodes::recipe_query(&std::collections::HashMap::from([
        ("cmd".to_string(), "import_recipe".to_string()),
        ("source_path".to_string(), tmp_str),
        ("target_dir".to_string(), "__root__".to_string()),
        ("overwrite".to_string(), "false".to_string()),
    ])).await;

    // 清理
    let _ = std::fs::remove_file(&tmp_path);
    result.map(|_| ())
}

// ═══════════════════════════════════════════════════════════════
// 版本检查 — GitHub release 更新提示
// ═══════════════════════════════════════════════════════════════

#[derive(Debug, Clone, Serialize)]
pub struct UpdateInfo {
    pub current: String,
    pub latest: String,
    pub has_update: bool,
    pub published_at: Option<String>,
    pub html_url: Option<String>,
    pub body: Option<String>,
}

#[tauri::command]
pub async fn check_latest_version() -> Result<UpdateInfo, String> {
    let current = env!("CARGO_PKG_VERSION").to_string();

    let client = reqwest::Client::builder()
        .timeout(std::time::Duration::from_secs(15))
        .build()
        .map_err(|e| format!("创建 HTTP 客户端失败: {}", e))?;

    let api_url = "https://api.github.com/repos/KnotLink-Protocol/KnotHub/releases/latest";

    let resp = client
        .get(api_url)
        .header("User-Agent", "KnotHub-Dash")
        .header("Accept", "application/vnd.github+json")
        .send()
        .await
        .map_err(|e| format!("网络请求失败: {}", e))?;

    if !resp.status().is_success() {
        // 网络不通或 API 异常 → 不提示，返回相同版本
        return Ok(UpdateInfo {
            current,
            latest: String::new(),
            has_update: false,
            published_at: None,
            html_url: None,
            body: None,
        });
    }

    let json: serde_json::Value = resp.json().await
        .map_err(|_| "解析 JSON 失败".to_string())?;

    let tag = json["tag_name"].as_str().unwrap_or("").to_string();
    let html = json["html_url"].as_str().map(String::from);
    let published = json["published_at"].as_str().map(String::from);
    let body = json["body"].as_str().map(String::from);

    // 版本比较
    let latest = tag.trim_start_matches('v');
    let has_update = is_version_newer(latest, &current);

    Ok(UpdateInfo {
        current,
        latest: tag,
        has_update,
        published_at: published,
        html_url: html,
        body,
    })
}

/// 去 v 前缀后三段数字比较，latest > current 返回 true
fn is_version_newer(latest: &str, current: &str) -> bool {
    let to_nums = |v: &str| -> Vec<u32> {
        v.replace(|c: char| !c.is_ascii_digit() && c != '.', "")
            .split('.')
            .filter_map(|s| s.parse().ok())
            .collect()
    };
    let l = to_nums(latest);
    let c = to_nums(current);
    if l.is_empty() || c.is_empty() { return false; }
    for i in 0..l.len().max(c.len()) {
        let a = l.get(i).copied().unwrap_or(0);
        let b = c.get(i).copied().unwrap_or(0);
        if a > b { return true; }
        if a < b { return false; }
    }
    false
}
