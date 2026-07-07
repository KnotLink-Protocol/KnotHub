# CLAUDE.md

## 项目概述

KnotHub 是 **KnotLink** 生态系统中的服务中枢/守护进程。KnotLink 是基于 TCP 的
自定义通信总线，KnotHub 负责发现、注册、展示和编排各类"节点"（软件组件）。

```
                    ┌──────────────────────┐
                    │   KnotHubDash         │  Tauri v2 + React 19
                    │   (桌面仪表板)          │  TypeScript + Vite 7
                    └──────────┬───────────┘
                               │ TCP :6376 (OpenSocket 查询)
                    ┌──────────▼───────────┐
                    │   KnotHubCore          │  C++ / Qt 5.9.0
                    │   (守护进程)            │  MinGW 32-bit 静态编译
                    └──────────┬───────────┘
                               │ 管理三种节点
            ┌──────────────────┼──────────────────┐
            ▼                  ▼                  ▼
     PluginManager      StandaloneManager    RecipeManager
     (0x00000011)        (0x00000012)        (0x00000013)
     插入式节点            独立式节点            配方
```

## 项目结构

```
KnotHub/
├── .gitignore                            # 排除 build-*/、node_modules、staging/、bin/
├── CLAUDE.md
├── scripts/
│   ├── nsis/
│   │   ├── KnotHub.nsi                 # NSIS 安装脚本（用户级，合并分发）
│   │   └── build.bat                   # 收集产物 + 调 makensis
│   ├── staging/                        # gitignore — 打包前暂存 exe
│   ├── bin/                            # gitignore — 安装包输出
│   └── standalone-register.ps1         # 独立式节点注册表脚本（很少用）
│
├── KnotHubCore/src/KnotHubCore/
│   ├── main.cpp                        # 入口：托盘（默认）/控制台/服务三模式
│   ├── daemon.cpp/h                    # 核心协调器，持有三个 Manager
│   ├── knothubcore.cpp/h/ui            # 日志窗口 + 系统托盘
│   ├── NodeManager/
│   │   ├── nodemanager.cpp/h           # PluginManager — Plugins/ 下的节点
│   │   ├── standalonemanager.cpp/h     # StandaloneManager — 注册表发现的节点
│   │   ├── nodeloader.cpp/h            # QProcess 封装（taskkill /F /T 杀进程树）
│   │   └── plugininfo.cpp/h            # 插件元数据 + JSON 序列化
│   ├── RecipeManager/
│   │   └── recipemanager.cpp/h         # Python 配方文件树 + 执行
│   └── KnotLinkLib/                    # C++ KnotLink 通信库
│       ├── tcpclient.cpp/h             # TCP（4 字节大端长度前缀帧）
│       ├── opensocketquerier.cpp/h      # 请求-回复客户端
│       ├── opensocketresponser.cpp/h    # 请求-回复服务端
│       └── kludf.cpp/h                 # KLKVMap 键值序列化（key=val;key=val）
│
└── KnotHubDash/knot-hub-dash/
    ├── src/                            # React 前端
    │   ├── main.tsx / App.tsx / App.css
    │   ├── pages/
    │   │   ├── Home.tsx                # 仪表板概览 + 统计卡片
    │   │   ├── Nodes.tsx               # 插件/独立式节点（拖拽安装 .zip）
    │   │   ├── Interconnect.tsx         # 配方文件树（运行/停止/新建）
    │   │   ├── ServiceStatus.tsx        # 四端口 TCP 监控
    │   │   ├── Settings.tsx             # 自启开关 + 端口状态 + 关于
    │   │   └── Debug.tsx                # 开发者工具
    │   └── components/
    │       ├── ThemeToggle/             # 浅色/深色
    │       ├── FunctionListParser/      # 交互式功能调用器 + 文档视图
    │       └── preview/                 # 右侧预览面板
    └── src-tauri/                      # Rust/Tauri 后端
        ├── main.rs                     # 托盘图标、命令注册
        ├── nodes.rs                    # 插件 + 独立式 + 自启 Tauri 命令
        ├── recipes.rs                  # 配方 Tauri 命令
        └── knotlink_lib/               # C++ 库的 Rust 移植
            ├── tcp_client.rs / open_socket_querier.rs / ...
            └── klkvmap.rs
```

## 技术栈

| 层 | 技术 |
|----|------|
| 后端 | C++ / Qt 5.9.0 / MinGW 32-bit / qmake（静态编译） |
| 桌面壳 | Tauri v2 / Rust / Tokio |
| 前端 | React 19 / TypeScript 5.8 / Vite 7 / CSS Modules |
| 通信 | 自定义 KnotLink TCP 协议 |
| 序列化 | `key=value;key=value` 格式（KLKVMap）/ JSON |
| 打包 | NSIS（合并 Core + Dash，用户级安装） |

## KnotLink 通信协议

### 四端口架构

| 端口 | 服务 | 用途 |
|------|------|------|
| `6378` | OpenSocket | 回答者注册（C++ OpenSocketResponser 连这里） |
| `6376` | OpenSocket | 查询请求（Rust OpenSocketQuerier 连这里） |
| `6372` | Signal | 订阅注册 |
| `6370` | Signal | 信号发送 |

### 地址模型
- `AppID`：应用标识（KnotHubCore = `0x00000002`）
- `OpenSocketID`：服务标识（每个 Manager 一个）
- 路由 key：`{AppID}-{OpenSocketID}&*&{payload}`

### SocketID 分配
| SocketID | Manager | 职责 |
|----------|---------|------|
| `0x00000011` | PluginManager | 插入式节点 CRUD + 启停 + 拖拽安装 |
| `0x00000012` | StandaloneManager | 独立式节点发现 + 列表（不管理进程） |
| `0x00000013` | RecipeManager | 配方文件树 + CRUD + 执行 |

### 消息格式
- TCP 帧：4 字节大端长度前缀 + 负载
- 负载：`{AppID}-{SocketID}&*&cmd=xxx;key1=val1;key2=val2`
- 最大消息：16MB

### Rust ↔ C++ 通信链路
1. Rust `OpenSocketQuerier` 连接 `127.0.0.1:6376`
2. C++ `OpenSocketResponser` 连接 `127.0.0.1:6378`
3. KnotLink 总线在中间做路由

## 三类节点

### 1. 插入式节点 (Plugin)
- **来源**：`Plugins/` 目录，每个子文件夹含 `plugin_manifest.json`
- **管理**：PluginManager 负责发现、启停、自动启动
- **生命周期**：KnotHubCore 管理进程（QProcess + taskkill /F /T）
- **安装**：前端拖拽 `.zip` → PluginManager 用 quazip 解压 → 刷新列表

### 2. 独立式节点 (Standalone)
- **来源**：Windows 注册表 `HKCU\Software\KnotLink\StandaloneNodes`
  - 值名 = app_id，值数据 = 安装目录
- **清单**：安装目录下的 `standalone_manifest.json`（由软件自行提供）
- **管理**：StandaloneManager **只做发现和列表导出，不管理进程**
- **状态**：固定显示"已注册"
- **动态刷新**：`get_standalone_list` 每次都重新扫描注册表

### 3. 配方 (Recipe)
- **来源**：`Recipes/` 目录树中的 `.py` 和 `.kln`（ZIP 包，运行时解压）
- **管理**：RecipeManager 支持文件树 CRUD + 文件夹管理 + 拖拽导入 + 执行
- **执行**：`python <script>` 或解压 `.kln` 后运行 `main.py`
- **导入**：拖 `.py`/`.kln` 到页面 → `import_recipe` 命令复制到 `Recipes/`，`__root__` 映射为根目录
- **文件夹**：新建时名称以 `/` 结尾 → `create_folder` 命令创建目录

## 插件清单格式

```json
{
    "plugin_name": "消息提醒",
    "app_id":      "0x00000014",
    "author":      "HXH",
    "version":     "v1.0.0",
    "description": "在屏幕顶端弹出消息窗口",
    "auto_start":  "true",
    "exe_path":    "MsgNotification.exe"
}
```

Zip 包直接就是插件文件夹的内容：`plugin_manifest.json` + `FuncList.json`（可选）+ exe 文件。

## 独立式注册表 + 清单

```
HKCU\Software\KnotLink\StandaloneNodes
  └── MyApp = "C:\Program Files\MyApp"
```

```json
{
  "app_id": "my.app.id",
  "app_name": "My Application",
  "author": "author",
  "version": "1.0.0",
  "description": "...",
  "auto_start": "true",
  "exe_path": "bin\\myapp.exe"
}
```

## 常用命令

```bash
# 前端开发（在 KnotHubDash/knot-hub-dash 下）
npx tauri dev        # Vite + Rust 后端（注意先杀端口 1420 的残留进程）
npm run dev          # 仅 Vite

# 前端 release 构建
npx tauri build      # 输出：src-tauri/target/release/bundle/

# C++ 静态构建（Qt Creator 打开 KnotHubCore.pro，选 static-Release kit）
# 产物：build-KnotHubCore-static-Release/release/KnotHubCore.exe（单文件，~21MB）

# 打包安装包（先确保 staging/ 里有两个 exe）
cd scripts/nsis
makensis KnotHub.nsi
# 输出：../bin/KnotHub-1.0.0.0-Setup.exe
```

## 打包分发

### 构建产物

| 产物 | 路径 | 说明 |
|------|------|------|
| KnotHubCore.exe | `build-KnotHubCore-static-Release/release/` | Qt 静态编译 ~21MB，零 DLL |
| knot-hub-dash.exe | `src-tauri/target/release/` | Tauri release ~10MB，零运行时 |

### 合并安装包

```
C:\Users\<name>\AppData\Local\Programs\KnotHub\
├── KnotHubCore.exe         # 默认托盘模式（日志窗口 + 系统托盘）
├── knot-hub-dash.exe       # 桌面仪表板
├── uninst.exe
├── Plugins\               # 拖拽 .zip 安装到这儿
└── Recipes\               # 配方文件放这儿
```

- **用户级安装**：不需要管理员权限
- **Core 自启**：`Exec` 安装后立即启动 + `HKCU\...\Run` 登录自启
- **Dash 快捷方式**：开始菜单 `SMPROGRAMS` + 桌面
- **卸载**：taskkill 杀进程 → 清理注册表 → 删除文件目录
- **升级检测**：`.onInit` 检查已有版本，提示卸载后继续

### 安装包大小

| 内容 | 原始 | 压缩后 |
|------|------|--------|
| KnotHubCore.exe | 21MB | — |
| knot-hub-dash.exe | 10MB | — |
| **总计** | ~31MB | **~12MB** (zlib) |

## NSIS 注意事项

- 脚本必须是 **ACP 编码**（不能是 UTF-8），否则 makensis 报 `Bad text encoding`
- 用户级安装需 `RequestExecutionLevel user` + `SetShellVarContext current`
- `$DESKTOP` / `$SMPROGRAMS` 不设 `SetShellVarContext current` 会写到公共目录（需管理员权限）
- 版本信息缺少 `LegalCopyright` 只会 warning，不影响构建

## 拖拽安装模式

两个页面共用相同模式，基于 Tauri `onDragDropEvent`：

### 插件安装 (Nodes.tsx)
```
拖 .zip → PluginManager.install_plugin(zip_path)
  → quazip 解压到 Plugins/<plugin_name>/
  → 验证 plugin_manifest.json → refreshPluginList() → 返回 ok
  → 前端自动刷新列表
```

### 配方导入 (Interconnect.tsx)
```
拖 .py/.kln →
  if 悬停文件夹 → target_dir = 文件夹绝对路径
  else          → target_dir = tree.id (__root__)
  → RecipeManager.import_recipe(source, target, overwrite)
  → QFile::copy → 刷新树 → 返回 ok
  文件已存在 → 返回 "error: file exists" → 前端弹窗询问覆盖
```

### 拖拽 UI 状态
- `dragOver: boolean` — 文件进入窗口时变 true，显示蓝色虚线覆盖层
- `dragTarget: string` — 悬停在文件夹行上时高亮该行，显示 "📥 放到这里"
- `importMsg: string | null` — 导入结果提示条（蓝底成功 / 红底失败）

## KL 命令速查

### PluginManager (0x00000011)
| 命令 | 参数 | 说明 |
|------|------|------|
| `get_plugin_list` | — | 返回插件列表 JSON |
| `get_plugins_root` | — | 返回 Plugins 目录绝对路径 |
| `get_detail` | plugin_name | 返回单个插件详情 |
| `plugin_control` | action, plugin_name | start / stop / restart |
| `update_config` | plugin_name, autostart | 修改自启配置 |
| `get_funclist` | plugin_name | 返回 FuncList.json 内容 |
| `install_plugin` | zip_path | 解压 zip 到 Plugins/ |
| `refresh` | — | 重新扫描并返回列表 |

### StandaloneManager (0x00000012)
| 命令 | 参数 | 说明 |
|------|------|------|
| `get_standalone_list` | — | 扫描注册表并返回列表 |
| `get_detail` | plugin_name | 返回单个节点详情 |
| `get_funclist` | plugin_name | 返回 FuncList.json |
| `refresh` | — | 重新扫描注册表 |

### RecipeManager (0x00000013)
| 命令 | 参数 | 说明 |
|------|------|------|
| `get_recipe_tree` | — | 返回目录树 JSON |
| `get_recipes_root` | — | 返回 Recipes 目录绝对路径 |
| `recipe_run` | file_path | 执行 Python 配方 |
| `recipe_stop` | file_path | 停止配方 |
| `recipe_status` | file_path | 返回 running/stopped |
| `recipe_read` | file_path | 读取文件内容 |
| `recipe_save` | file_path, content | 创建/覆盖文件 |
| `recipe_delete` | file_path | 删除文件或目录 |
| `import_recipe` | source_path, target_dir, overwrite | 复制文件到 Recipes/ |
| `create_folder` | path | 创建目录 |

## Rust Tauri 命令

除上述 KL 命令的 Tauri 包装外，还有本地命令：

| 命令 | 参数 | 说明 |
|------|------|------|
| `open_folder` | path | explorer 打开任意目录 |
| `open_app_dir` | sub | 打开 exe 同目录下的子目录（如 Recipes），不存在则创建 |
| `get_core_autostart` | — | 检查 HKCU Run 中是否有 KnotHub |
| `set_core_autostart` | enable | 写/删 HKCU Run |
| `get_knotlink_addr` | — | 返回 127.0.0.1:6376 |
| `check_service_port` | addr | TCP 连接检测端口是否在线 |

> 注意：`open_folder` 用 `explorer <path>` 打开目录。若路径不存在，explorer 会打开"文档"文件夹而非报错，所以 `open_app_dir` 会先 `create_dir_all` 确保目录存在。

## NSIS 注意事项

- 脚本必须是 **ACP 编码**（不能是 UTF-8），否则 makensis 报 `Bad text encoding`
- 用户级安装需 `RequestExecutionLevel user` + `SetShellVarContext current`
- `$DESKTOP` / `$SMPROGRAMS` 不设 `SetShellVarContext current` 会写到公共目录（需管理员权限）
- 版本信息缺少 `LegalCopyright` 只会 warning，不影响构建

## 关键设计决策

1. **三层架构隔离**：三种节点类型各自独立的 SocketID 和 Manager
2. **独立式不管理进程**：独立式应用自行启停，StandaloneManager 只发现和展示
3. **动态刷新**：`get_standalone_list` 每次扫描注册表，`get_plugin_list` 返回缓存（需 `refresh` 命令刷新）
4. **进程树清理**：NodeLoader 使用 `taskkill /F /T /PID` 完整清理，包括子进程
5. **托盘最小化**：关闭窗口 → 隐藏到托盘，不是退出
6. **合并分发**：Core + Dash 一个 NSIS 包，用户级安装，Core 通过 HKCU Run 后台自启
7. **Core 默认托盘模式**：有日志窗口（深色 QPlainTextEdit，带时间戳）+ 系统托盘图标，双击托盘切换显示/隐藏
8. **插件安装走后端解压**：前端只传 zip 路径，PluginManager 用 quazip 解压到 Plugins/，验证 manifest 后刷新
9. **静态编译无依赖**：Core 静态链接 Qt + quazip + zlib，Dash 是单文件 Tauri 产物，两个 exe 都不需要额外运行时
