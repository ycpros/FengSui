# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

烽燧 (FengSui) — 纯 Qt 6 + C++17 单进程桌面客户端，无中心服务器的局域网 IM 与文件协作工具。目标平台 Windows / Ubuntu / Debian / openKylin x86_64 / macOS。

## 构建命令

```bash
# 配置（Debug，使用 Ninja 生成器）
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 构建
cmake --build cmake-build-debug

# 运行
./cmake-build-debug/fengsui.exe   # Windows
./cmake-build-debug/fengsui       # Linux/macOS
```

已配置的 cmake-build-debug 目录可直接构建。CLion 2026.1.1 项目也可直接用 IDE 构建。

编译命令可从 `cmake-build-debug/compile_commands.json` 获取（已为 clangd 等工具生成）。

## 架构：四层 + 共享模型

```
ui/          → Qt Widgets（MainWindow + 各功能页面），只能调用 core/ 的 Service
core/        → 核心业务逻辑（BeaconService 等），通过 signal/slot 通知 UI，不包含 QWidget
network/     → 底层 socket 管理，不操作 UI 和数据库
storage/     → 封装所有 SQLite 操作（Database + 各 Repository），对上层返回 models/ 结构体
models/      → 纯头文件 struct，可被所有层 include
```

应用入口 `src/main.cpp` 通过 `FengSui::Application` 管理初始化顺序：日志 → 数据库 → 设置仓库 → 设置 → 向导检查 → BeaconService → 主窗口。

## 关键约束

- **C++17**，Qt 6.8+，CMake 3.20+
- **仅 Qt Widgets**，禁止 QML / Qt Quick / Qt WebEngine / QWebChannel
- **仅用 Qt 模块**：Widgets、Network、Sql（加自动的 Core）
- **单进程**，无 IPC、无后台 daemon
- **序列化**：QJsonDocument，禁止 Protobuf / MessagePack
- **V1.0 无加密**：明文传输，禁止引入 OpenSSL / TLS
- **禁止第三方网络库**（Boost.Asio 等），仅用 Qt Network 的 QTcpSocket / QUdpSocket
- **禁止引入新文件目录结构**，严格遵循 `docs/04_architecture.md` 的定义
- `storage/` 是 SQLite 唯一出口；`ui/` 层不得直接操作数据库或 socket

## 命名规范

| 类型 | 规范 | 示例 |
|---|---|---|
| 命名空间/类名 | PascalCase | `FengSui`, `BeaconService` |
| 函数/方法/局部变量 | camelCase | `sendMessage()`, `peerCount` |
| 成员变量 | `m_` 前缀 + camelCase | `m_peerId` |
| Signal | `on + 过去式` 或 `名词 + Changed` | `peerOnline`, `transferProgress` |
| Slot | `on + 描述` | `onSendClicked`, `onMessageReceived` |
| 文件名 | PascalCase，与类名一致 | `BeaconService.h`, `MainWindow.cpp` |

- 所有头文件用 `#pragma once`
- `#include` 顺序：自身头 → Qt 头 → 项目内部（用 `"core/BeaconService.h"` 相对路径）→ 标准库
- 禁止在头文件中 `using namespace`

## 注释规则

全部使用**中文注释**，仅用 `//` 行注释（禁止 `/* */`）。公开 API 必须有注释；行内注释解释"为什么"而非"是什么"。TODO 格式：`// TODO(用户名): 描述`。

## 文档参考

项目文档在 `docs/` 下，实施前优先参考：
- `03_mvp_scope.md` — P0/P1 功能与验收标准
- `04_architecture.md` — 分层规则与数据流
- `10_coding_rules.md` — 完整编码规范
- `12_agent_instructions.md` — Agent 行为准则
- `09_task_breakdown.md` — 当前任务分解
