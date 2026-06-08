# 04 — 技术架构设计

## 架构总览

V1.0 采用 **纯 Qt 6 + C++ 单进程架构**。无 IPC、无后台服务进程、无额外核心依赖。

```
┌──────────────────────────────────┐
│         Qt Widgets UI            │
│  mainwindow / chat / transfer    │
│  share / settings / onboarding   │
├──────────────────────────────────┤
│      Application Services        │
│  BeaconService / SignalService   │
│  CourierService / ShareService   │
├──────────────────────────────────┤
│         Local Storage            │
│  SQLite (QSqlDatabase) / FS      │
└──────────────────────────────────┘
```

## 源码目录结构

```
src/
├── main.cpp                     # 入口，QApplication 初始化
├── app/                         # 应用层（全局配置、日志、App 实例）
│   ├── Application.h/cpp        # QApplication 子类
│   ├── AppSettings.h/cpp        # 配置读写（SQLite settings 表门面）
│   └── Logger.h/cpp             # 日志系统
├── ui/                          # UI 层（所有 Widget）
│   ├── MainWindow.h/cpp         # 主窗口，左侧导航 + 右侧页面栈
│   ├── onboarding/              # 首次启动向导
│   ├── chat/                    # 聊天窗口、消息列表、输入框
│   ├── contacts/                # 联系人/设备列表
│   ├── transfer_center/         # 传输中心面板
│   ├── share/                   # 共享文件浏览、我的共享管理
│   ├── dropbox/                 # 投递箱列表、新建投递
│   ├── settings/                # 设置页（网络发现、下载目录等）
│   ├── diagnostics/             # 网络诊断页
│   └── tray/                    # 系统托盘
├── core/                        # 核心业务逻辑层（与 UI 解耦）
│   ├── BeaconService.h/cpp      # UDP 发现服务
│   ├── SignalService.h/cpp      # TCP 消息通道服务
│   ├── CourierService.h/cpp     # 文件传输服务
│   ├── ShareService.h/cpp       # 共享目录服务
│   └── DiagnosticsService.h/cpp # 诊断逻辑
├── network/                     # 网络协议与通信
│   ├── UdpDiscovery.h/cpp       # UDP 广播/多播收发
│   ├── TcpConnection.h/cpp      # TCP 连接管理
│   ├── TcpServer.h/cpp          # TCP 监听服务
│   ├── HttpServer.h/cpp         # P1 浏览器访客模式 HTTP 服务
│   └── Protocol.h/cpp           # 协议消息序列化/反序列化
├── storage/                     # 数据持久化层（唯一访问 SQLite 的层）
│   ├── Database.h/cpp           # SQLite 初始化、连接管理
│   ├── PeerRepository.h/cpp     # 设备信息 CRUD
│   ├── ConversationRepository.h/cpp
│   ├── MessageRepository.h/cpp
│   ├── TransferRepository.h/cpp
│   ├── ShareRepository.h/cpp
│   └── Migration.h/cpp          # 数据库迁移
├── models/                      # 纯数据结构（无逻辑）
│   ├── PeerInfo.h
│   ├── Message.h
│   ├── Conversation.h
│   ├── TransferTask.h
│   ├── SharedFolder.h
│   ├── AccessGrant.h
│   └── AppSettings.h
└── platform/                    # 平台特有代码
    ├── PlatformUtils.h/cpp      # 平台检测、路径、主机名
    └── FirewallChecker.h/cpp    # 防火墙检测（各平台实现）
```

## 分层规则（禁止跨界）

| 层 | 允许 | 禁止 |
|---|---|---|
| `ui/` | 调用 `core/` 的 Service 类 public 方法；通过 signal/slot 接收事件 | 直接调用 `network/` 的 socket 类；直接调用 `storage/` 的 Repository；执行阻塞 IO |
| `core/` | 调用 `network/` 和 `storage/`；通过 signal 通知 UI 状态变化 | 直接操作 UI 控件（`#include <QWidget>` 只能在 ui/） |
| `network/` | 管理 socket、序列化协议、收发数据 | 直接操作 UI 控件；直接操作数据库 |
| `storage/` | 封装所有 SQLite 操作，对外返回 models/ 的结构体 | 直接操作 socket 或 UI |
| `models/` | 纯头文件 struct，可被所有层 include | 不依赖 Qt（除 QString/QDateTime 外） |

## 构建系统

- **构建工具**：CMake 3.20+
- **Qt 版本**：Qt 6.8+（核心模块：Widgets、Network、Sql）
- **C++ 标准**：C++17
- **打包**：CPack（Windows NSIS、Linux deb、macOS dmg）

CMakeLists.txt 顶层结构：
```cmake
cmake_minimum_required(VERSION 3.20)
project(FengSui VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)
add_executable(fengsui ...)
target_link_libraries(fengsui Qt6::Widgets Qt6::Network Qt6::Sql)
```

## 禁止引入的技术清单

- Rust（任何形式）
- Asio / Boost.Asio（V1.0 不用）
- Electron / NW.js / 任何 Web 前端框架
- QML / Qt Quick
- Flutter
- 任何需要额外系统服务的依赖（Avahi、Bonjour 等 mDNS 守护进程）
- OpenSSL / TLS（V1.0 明文传输，后续版本启用）

## 数据流

**消息发送流程：**
```
UI (回车) → SignalService::sendMessage()
  → TcpConnection::send(Protocol::serialize(msg))
  → 对方 TcpServer::onReadyRead()
  → SignalService::onMessageReceived()
  → Storage::MessageRepository::save(msg)
  → 通过 signal 通知 UI 更新
```

**文件传输流程：**
```
UI (拖拽文件) → CourierService::sendFile(peerId, path)
  → TcpConnection::send(transfer.request JSON)
  → 对方 CourierService::onTransferRequest()
  → UI (弹窗：接收/拒绝)
  → 接受后：CourierService → TcpConnection 分块发送
  → 对方 CourierService 分块接收 → 写入文件
  → 进度通过 signal 通知双方 UI
  → 完成后 Storage::TransferRepository::save(task)
```
