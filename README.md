# 烽燧 FengSui

烽燧 FengSui 是一个 **纯 Qt 6 + C++17** 的桌面端局域网 IM 与文件协作客户端，面向无外网、弱外网或不方便部署中心服务器的同网段协作场景。

它的目标不是替代企业微信、OA 或网盘平台，而是先把局域网内最常见的几件事做顺：看见在线设备、发消息、传大文件、共享目录、授权访问、查看传输记录，以及在网络不通时自助诊断。

## 项目状态

当前项目处于 MVP 开发阶段，工程骨架与部分 P0 功能已经落地：

- 已有 Qt Widgets 主窗口、左侧导航和页面栈。
- 已有首次启动向导：昵称、网络发现开关、默认下载目录。
- 已有联系人列表和手动添加设备入口。
- 已有基础会话、消息持久化、消息状态更新和聊天 UI。
- 已有 UDP 发现、TCP 通信、SQLite 存储、QtTest 测试目标等工程基础。
- 传输中心、共享文件、设置、网络诊断、投递箱等页面已建入口或占位，目标态见文档和原型。

详细范围以 [MVP 功能清单](docs/03_mvp_scope.md) 为准。

## 核心能力

### P0 首发能力

| 能力 | 说明 |
|---|---|
| 设备发现 | 同网段 UDP 自动发现在线设备 |
| 手动 IP | UDP 发现不可用时，输入 IP:端口 直连 |
| 单聊消息 | 一对一文本消息，本地 SQLite 持久化 |
| 文件传输 | 支持文件和文件夹拖拽发送，记录传输状态 |
| 传输中心 | 查看进行中、已完成、失败和取消的传输任务 |
| 共享文件夹 | 将本地目录共享给同网段设备浏览和下载 |
| 访问授权 | 首次访问共享时弹窗确认，可拒绝或记住选择 |
| 网络诊断 | 检查 IP、端口、防火墙提示、UDP 广播和在线设备数 |

### P1 后续增强

- 广播通知与确认回执
- 文件投递箱
- 断点续传
- 浏览器访客模式
- 系统托盘增强

## 适用场景

- 断网研发网、测试网、实验室网
- 办公室同网段 Windows 设备之间的消息与文件流转
- 设计、视频、教务等跨 Windows / Linux / macOS 的资料交换
- 外协或访客临时提交、下载资料
- 10-100 人小型内网团队的轻量文件协作

## 非目标

MVP 阶段明确不做：

- 音视频会议
- 在线文档协作
- 组织架构、审批、OA
- 互联网跨网远程传输
- 真正的云离线消息
- 跨 VLAN / 跨子网自动发现
- Android / iOS 移动端

## 技术栈

- 语言：C++17
- UI：Qt 6 Widgets
- 网络：Qt Network，基于 UDP / TCP
- 存储：SQLite，使用 Qt Sql
- 构建：CMake 3.20+
- 测试：QtTest + CTest
- 目标平台：Windows、Ubuntu/Debian、openKylin x86_64、macOS

V1.0 采用单进程桌面客户端架构，不使用 QML、Qt Quick、Electron、Flutter、Web 前端框架、第三方网络库或额外后台服务。

## 快速开始

### 环境要求

- Qt 6.8+
- CMake 3.20+
- 支持 C++17 的编译器
- Ninja 可选，但推荐用于本地 Debug 构建

### 配置

```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### 构建

```bash
cmake --build cmake-build-debug
```

### 运行

Windows:

```powershell
.\cmake-build-debug\fengsui.exe
```

Linux / macOS:

```bash
./cmake-build-debug/fengsui
```

### 测试

```bash
ctest --test-dir cmake-build-debug
```

当前 CMake 已注册的测试目标包括协议、存储和消息服务相关测试。

## 工程结构

```text
src/
├── app/                  # 应用初始化、设置、日志
├── core/                 # 业务服务：发现、消息、传输等
├── models/               # 共享数据结构
├── network/              # UDP / TCP 通信与协议序列化
├── platform/             # 平台相关工具
├── storage/              # SQLite 数据库与 Repository
├── tests/                # QtTest 测试
└── ui/                   # Qt Widgets 页面与窗口
```

核心分层规则：

- `ui/` 只负责界面展示和用户操作，调用 `core/` 的 Service。
- `core/` 承载业务逻辑，可调用 `network/` 和 `storage/`，但不直接操作 QWidget。
- `network/` 只负责 socket、连接和协议数据，不操作 UI 或数据库。
- `storage/` 是 SQLite 的唯一出口，对外返回 `models/` 数据结构。
- `models/` 保持轻量，供各层共享。

更多细节见 [技术架构设计](docs/04_architecture.md)。

## 安全与边界

V1.0 面向可信局域网环境，当前传输为明文，不提供加密传输能力。发现能力只解决“看到同网段设备”，访问共享目录仍需要用户授权确认。

如果用于保密、合规或跨组织协作场景，应在后续版本中补充更完整的身份认证、加密传输、审计和管理策略。

## 原型与文档

- [项目总览](docs/01_project_brief.md)
- [PRD](docs/02_prd.md)
- [MVP 功能清单](docs/03_mvp_scope.md)
- [技术架构设计](docs/04_architecture.md)
- [UI 交互规格](docs/08_ui_spec.md)
- [测试计划](docs/11_test_plan.md)
- [高保真产品原型](prototype/index.html)

## 开发约束

- 使用 Qt Widgets，不使用 QML / Qt Quick / Qt WebEngine。
- 网络层仅使用 Qt Network。
- 序列化使用 `QJsonDocument`。
- UI 层不得直接操作 socket 或 SQLite。
- storage 层统一封装数据库访问。
- 公开 API 注释使用中文，头文件使用 `#pragma once`。

完整编码规范见 [编码规则](docs/10_coding_rules.md)。

## 许可证

仓库当前未提供 LICENSE 文件。正式对外发布或开源前，请先补充明确的许可证。
