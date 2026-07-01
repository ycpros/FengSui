# 烽燧 FengSui

[English](README.md) | 简体中文

烽燧 FengSui 是一个基于 Qt 6 和 C++17 的桌面端局域网消息与文件协作客户端。
它面向可信本地网络：团队可以在不部署中心服务器、不依赖外网的情况下发现同网段设备、发送消息、传输文件和共享目录。

烽燧并不试图替代企业 IM、网盘或 OA 系统。它的目标更窄：把“同一个网络里，需要沟通和交换文件”的常见流程做得简单、可检查、以本地优先。

## 项目状态

烽燧目前处于早期 MVP 开发阶段。仓库中已经包含可运行的 QML/Qt Quick 应用外壳、本地持久化、局域网发现与 TCP 消息基础、文件传输流程、共享文件夹浏览，以及覆盖核心行为的 QtTest 测试。

项目尚未发布稳定版本。v1.0 前协议、界面细节、数据库表和工作流都可能继续调整。

## 功能

- 基于 UDP 广播的同网段设备发现。
- 当广播发现不可用时，可通过 IP 地址和端口手动连接设备。
- 一对一文本会话，并使用本地 SQLite 持久化。
- 基于 TCP 的单文件传输，支持接受、拒绝、取消、进度和历史记录。
- 传输中心页面，可筛选和查看传输任务。
- 本地共享文件夹发布、远端浏览、文件下载和本地访问授权。
- 首次启动向导，用于设置显示名称、发现偏好和默认下载目录。
- 设置与诊断页面，用于查看网络策略、网卡、手动连接记录和基础检查。
- 面向 Windows、Linux 和 macOS 的跨平台 QML/Qt Quick 界面。
- 基于 QtTest 和 CTest 的自动化测试。

## 当前限制

- 当前版本假设运行在可信局域网中。传输为明文，不提供端到端加密、强身份认证或审计级访问控制。
- 自动发现仅限本地子网。跨 VLAN 和互联网穿透不属于当前版本范围。
- 文件夹拖拽和递归文件夹传输尚未完成。
- 文件投递箱流程和增强系统托盘行为属于计划能力，尚未完整实现。
- 目前没有移动端、云中继服务、组织管理、音视频通话或在线文档协作能力。

## 技术栈

- 语言：C++17
- UI：Qt 6 QML / Qt Quick
- 网络：Qt Network，基于 UDP 和 TCP
- 存储：SQLite，通过 Qt SQL 访问
- 构建：CMake 3.20+
- 测试：QtTest 和 CTest
- 目标平台：Windows、Linux、macOS

v1 系列刻意不使用 Electron、Flutter、Web 前端框架、第三方网络库或额外后台服务依赖。Windows 上系统托盘通过 QtWidgets 作为 Qt.labs.platform/QSystemTrayIcon 的后端实现，应用界面仍保持 QML/Qt Quick。

## 从源码构建

### 环境要求

- Qt 6.8 或更新版本
- CMake 3.20 或更新版本
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

Linux 或 macOS:

```bash
./cmake-build-debug/fengsui
```

### 测试

```bash
ctest --test-dir cmake-build-debug --output-on-failure
```

## 仓库结构

```text
src/
├── app/        应用启动、设置和日志
├── core/       发现、消息、共享和传输等业务服务
├── models/     共享数据结构
├── network/    UDP/TCP 网络通信与协议序列化
├── platform/   平台相关工具
├── storage/    SQLite 数据库和 Repository
├── tests/      QtTest 测试目标
└── ui/         QML 页面、组件、对话框和 ViewModel
```

分层约定：

- `ui/` 负责界面展示和用户操作，并调用 `core/` 中的服务。
- `core/` 承载业务行为，可调用 `network/` 和 `storage/`。
- `network/` 负责 socket、连接和协议载荷。
- `storage/` 是唯一数据库访问层，并通过 `models/` 返回数据。
- `models/` 保持轻量，供各层共享。

## 参与贡献

项目仍在成型中，欢迎贡献。请尽量让变更保持聚焦，并符合当前架构：

- UI 开发使用 QML/Qt Quick。QtWidgets 仅作为 Windows 系统托盘后端链接。
- 网络能力使用 Qt Network。
- 协议序列化使用 `QJsonDocument` 及相关 Qt JSON 类型。
- 不在 `core/`、`network/`、`storage/` 中编写 UI 代码。
- 行为变更需要新增或更新 QtTest 覆盖。
- 优先提交清晰的局部变更，避免无关的大范围重构。

## 安全说明

烽燧面向可信本地网络。当前版本不适合直接用于保密、合规、敌对网络或跨组织协作环境；如果要用于这些场景，需要补充更强的身份认证、加密传输、访问审计和管理控制。

## 许可证

烽燧基于 MIT License 发布。详情见 [LICENSE](LICENSE)。
