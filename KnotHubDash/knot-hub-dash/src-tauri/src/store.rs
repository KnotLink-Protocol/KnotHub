use serde::{Deserialize, Serialize};

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

#[tauri::command]
pub async fn download_and_install(url: String) -> Result<(), String> {
    // 1. 解析下载链接（支持 GitHub releases/latest → 真实下载地址）
    let real_url = resolve_download_url(&url).await?;

    // 2. 下载
    let client = reqwest::Client::builder()
        .timeout(std::time::Duration::from_secs(120))
        .build()
        .map_err(|e| format!("创建 HTTP 客户端失败: {}", e))?;

    let response = client
        .get(&real_url)
        .send()
        .await
        .map_err(|e| format!("下载失败: {}", e))?;

    if !response.status().is_success() {
        return Err(format!("HTTP {}", response.status()));
    }

    let bytes = response
        .bytes()
        .await
        .map_err(|e| format!("读取下载内容失败: {}", e))?;

    if bytes.is_empty() {
        return Err("下载的文件为空".into());
    }

    // 3. 写临时文件
    let tmp = std::env::temp_dir()
        .join(format!("knothub_dl_{}.zip", std::process::id()));
    std::fs::write(&tmp, &bytes)
        .map_err(|e| format!("写入临时文件失败: {}", e))?;

    // 4. 调已有 install_plugin
    let tmp_str = tmp.to_string_lossy().to_string();
    let result = crate::nodes::install_plugin(tmp_str).await;

    // 5. 清理临时文件
    let _ = std::fs::remove_file(&tmp);

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
