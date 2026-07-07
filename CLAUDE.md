# CLAUDE.md

## 项目概述

KnotHub 是 **KnotLink** 生态系统中的服务中枢/守护进程。KnotLink
是基于TCP的自定义通信总线（端口6376/6378），KnotHub负责发现、注册、展示和编排
各类"节点"（软件组件）。

```
                    ┌──────────────────────┐
                    │   KnotHubDash         │  Tauri v2 + React 19
                    │   (桌面仪表板)          │  TypeScript + Vite 7
                    └──────────┬───────────┘
                               │ TCP :6376 (KnotLink 协议)
                    ┌──────────▼───────────┐
                    │   KnotHubCore          │  C++ / Qt 5.9.0
                    │   (守护进程)            │  MinGW 32-bit
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
├── KnotHubCore/src/KnotHubCore/
│   ├── main.cpp              # 入口：托盘/控制台/服务三模式
│   ├── daemon.cpp/h          # 核心协调器，持有三个 Manager
│   ├── NodeManager/
│   │   ├── nodemanager.cpp/h       # PluginManager — 管理 Plugins/ 下的节点
│   │   ├── standalonemanager.cpp/h # StandaloneManager — 注册表发现的节点
│   │   ├── nodeloader.cpp/h        # QProcess 封装（taskkill /F /T 杀进程树）
│   │   └── plugininfo.cpp/h        # 插件元数据
│   ├── RecipeManager/
│   │   └── recipemanager.cpp/h     # Python 配方执行
│   └── KnotLinkLib/                # C++ KnotLink 通信库
│       ├── tcpclient.cpp/h         # TCP 客户端（4字节大端长度前缀）
│       ├── opensocketquerier.cpp/h  # 请求-回复客户端
│       ├── opensocketresponser.cpp/h# 请求-回复服务端
│       └── kludf.cpp/h             # KLKVMap 键值序列化
│
└── KnotHubDash/knot-hub-dash/
    ├── src/                        # React 前端
    │   ├── pages/Home|Nodes|Interconnect|ServiceStatus|Settings|Debug.tsx
    │   └── components/ThemeToggle|FunctionListParser|preview/
    └── src-tauri/                  # Rust/Tauri 后端
        ├── main.rs                 # 托盘图标、命令注册
        ├── nodes.rs                # 节点 Tauri 命令
        ├── recipes.rs              # 配方 Tauri 命令
        └── knotlink_lib/           # C++ 库的 Rust 移植
```

## 技术栈

| 层 | 技术 |
|----|------|
| 后端 | C++ / Qt 5.9.0 / MinGW 32-bit / qmake |
| 桌面壳 | Tauri v2 / Rust / Tokio |
| 前端 | React 19 / TypeScript 5.8 / Vite 7 / CSS Modules |
| 通信 | 自定义 KnotLink TCP 协议（端口6376/6378） |
| 序列化 | `key=value;key=value` 格式（KLKVMap）/ JSON |

## KnotLink 通信协议

### 地址模型
- `AppID`：应用标识（KnotHubCore = `0x00000002`）
- `OpenSocketID`：服务标识（每个 Manager 一个）
- 路由 key：`{AppID}-{OpenSocketID}&*&{payload}`

### SocketID 分配
| SocketID | Manager | 职责 |
|----------|---------|------|
| `0x00000011` | PluginManager | 插入式节点 CRUD + 启停 |
| `0x00000012` | StandaloneManager | 独立式节点发现 + 列表 |
| `0x00000013` | RecipeManager | 配方文件树 + 执行 |

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

### 2. 独立式节点 (Standalone)
- **来源**：Windows 注册表 `HKCU\Software\KnotLink\StandaloneNodes`
  - 值名 = app_id，值数据 = 安装目录
- **清单**：安装目录下的 `standalone_manifest.json`（由软件自行提供）
- **管理**：StandaloneManager 只做发现和列表导出，不管理进程
- **状态**：固定显示"已注册"
- **动态刷新**：`get_standalone_list` 每次都重新扫描注册表

### 3. 配方 (Recipe)
- **来源**：`Recipes/` 目录树中的 `.py` 和 `.kln`（ZIP）
- **管理**：RecipeManager 负责文件 CRUD 和执行

## Standalone 注册表结构

```
HKCU\Software\KnotLink\StandaloneNodes
  ├── MyApp = "C:\Program Files\MyApp"
  └── AnotherNode = "D:\Tools\AnotherNode"
```

### standalone_manifest.json 格式
```json
{
  "app_id": "my.app.id",
  "app_name": "My Application",
  "author": "author",
  "version": "1.0.0",
  "description": "Description text",
  "auto_start": "true",
  "exe_path": "bin\\myapp.exe"
}
```

## 常用命令

```bash
# 前端开发（在 KnotHubDash/knot-hub-dash 下）
npm run dev          # 仅 Vite
npx tauri dev        # Vite + Rust 后端

# C++ 构建（Qt Creator 或命令行）
qmake && make        # KnotHubCore.pro
```

## 打包分发

### 构建产物

| 产物 | 路径 | 说明 |
|------|------|------|
| KnotHubCore.exe | `KnotHubCore/src/build-KnotHubCore-static-Release/release/` | Qt 静态编译，21MB，零 DLL |
| knot-hub-dash.exe | `KnotHubDash/knot-hub-dash/src-tauri/target/release/` | Tauri 单文件，零运行时 |

### 打包

```bash
# 一键构建 + 打包
cd scripts\nsis
build.bat
# 输出: scripts\bin\KnotHub-1.0.0.0-Setup.exe
```

### 安装行为

```
C:\Users\<name>\AppData\Local\Programs\KnotHub\
├── KnotHubCore.exe         # --console 模式，后台运行
├── knot-hub-dash.exe       # 桌面仪表板
├── uninst.exe
├── Plugins\
└── Recipes\
```

- **用户级安装**：不需要管理员权限
- **Core 自启**：写入 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`
- **Dash 快捷方式**：开始菜单 + 桌面
- **卸载**：taskkill 杀进程 + 清理注册表 + 删除文件目录

### 启动流程

```
用户登录 → Run key 触发 KnotHubCore.exe --console（后台）
         → 用户双击桌面的 KnotHub 快捷方式 → Dash 连接 127.0.0.1:6376
```

## 关键设计决策

1. **三层架构隔离**：2026-07-07 重构，三种节点类型各自独立的 SocketID 和 Manager
2. **独立式不管理进程**：独立式应用自行启停，StandaloneManager 只发现和展示
3. **动态刷新**：`get_standalone_list` 每次扫描注册表，不需要重启核心
4. **进程树清理**：NodeLoader 使用 `taskkill /F /T /PID` 完整清理
5. **托盘最小化**：关闭窗口 → 隐藏到托盘，不是退出
6. **合并分发 & 用户级安装**：Core + Dash 一个包，用户级安装不需要管理员权限，Core 通过 HKCU Run 后台自启
