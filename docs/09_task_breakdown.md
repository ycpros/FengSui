# 09 — 开发任务拆分

> 按顺序逐任务实施。每个任务完成后再开始下一个。
> 每任务完成后交付：修改文件列表 + 实现说明 + 测试方法 + 是否影响已有功能。
> 每任务实施期间，遵循[12_agent_instructions.md](./12_agent_instructions.md) 。

---

## 第 0 阶段：工程准备

### 任务 001 — 创建 CMake/Qt 项目骨架

| 项 | 内容 |
|---|---|
| 目标 | 创建可编译运行的 Qt Widgets 空壳 |
| 涉及文件 | `CMakeLists.txt`、`src/main.cpp`、`src/app/Application.h`、`src/app/Application.cpp` |
| 验收标准 | 1. `cmake -B build && cmake --build build` 成功。2. 启动后显示空白主窗口（标题"烽燧 FengSui"、尺寸 1100×720）。3. 窗口可正常关闭。 |
| 禁止 | 不要添加任何菜单、按钮、业务逻辑。 |

### 任务 002 — 建立目录结构与基础模块

| 项 | 内容 |
|---|---|
| 目标 | 创建完整的 `src/` 目录树和基础类骨架 |
| 涉及文件 | `src/app/Logger.*`、`src/app/AppSettings.*`、`src/storage/Database.*`、`src/models/` 下所有头文件 |
| 验收标准 | 1. 目录结构与 [04_architecture.md](./04_architecture.md) 一致。2. Logger 可输出到文件和控制台。3. Database 初始化 SQLite 并执行 schema 创建。4. 所有 models/ 头文件编译通过。 |
| 禁止 | 不要实现任何业务逻辑。 |

---

## 第 1 阶段：UI 壳子

### 任务 003 — 主窗口与左侧导航

| 项 | 内容 |
|---|---|
| 目标 | 实现主窗口框架：左侧导航 + 右侧页面栈 |
| 涉及文件 | `src/ui/MainWindow.*` |
| 验收标准 | 1. 左侧导航栏含 5 个入口：消息、联系人、传输中心、共享文件、设置。2. 点击导航切换右侧页面（使用 QStackedWidget）。3. 导航高亮当前选中项。 |
| 禁止 | 不要实现各页面的具体内容。不要添加托盘。 |

### 任务 004 — 首次启动向导

| 项 | 内容 |
|---|---|
| 目标 | 3 步向导页面，用户设置昵称、发现开关、下载目录 |
| 涉及文件 | `src/ui/onboarding/OnboardingWizard.*` |
| 验收标准 | 1. `onboarding_completed = false` 时启动向导。2. 用户可通过 3 步完成设置。3. 完成后设置写入 SQLite，`onboarding_completed = true`，主窗口打开。4. 中途关闭窗口不保存。 |
| 禁止 | 不要实现自动发现或网络功能。 |

### 任务 005 — UI 页面占位

| 项 | 内容 |
|---|---|
| 目标 | 为所有导航页面创建空白占位 Widget |
| 涉及文件 | `src/ui/chat/ChatPage.*`、`src/ui/contacts/ContactsPage.*`、`src/ui/transfer_center/TransferCenterPage.*`、`src/ui/share/SharePage.*`、`src/ui/settings/SettingsPage.*` |
| 验收标准 | 1. 5 个页面均有空白 Widget。2. 导航切换正常。3. 每个页面标题正确显示。 |
| 禁止 | 不要实现页面具体内容。 |

---

## 第 2 阶段：局域网发现

### 任务 006 — UDP 发现服务

| 项 | 内容 |
|---|---|
| 目标 | 实现 UDP 广播与接收，维护在线设备列表 |
| 涉及文件 | `src/network/UdpDiscovery.*`、`src/core/BeaconService.*` |
| 验收标准 | 1. 启动后每 5 秒发送 presence.hello。2. 收到其他设备的 hello 后加入 PeerInfo 列表。3. 15 秒未收到心跳标记离线。4. 退出时发送 presence.goodbye（尽力）。5. 提供 signal：`peerOnline(PeerInfo)`、`peerOffline(QString peerId)`。 |
| 禁止 | 不要引入 mDNS/Avahi/Bonjour 依赖。 |

### 任务 007 — 在线设备列表（联系人页）

| 项 | 内容 |
|---|---|
| 目标 | 联系人页实时显示在线设备 |
| 涉及文件 | `src/ui/contacts/ContactsPage.*` |
| 验收标准 | 1. 列表实时反映上线/离线。2. 显示昵称、设备名、OS 图标、在线状态。3. 空态提示。4. 双击设备 → 打开聊天窗口。 |
| 禁止 | 不要实现聊天功能。 |

### 任务 008 — 手动添加 IP

| 项 | 内容 |
|---|---|
| 目标 | 支持手动输入 IP:端口连接设备 |
| 涉及文件 | `src/ui/contacts/` 新增 AddPeerDialog |
| 验收标准 | 1. 输入合法 IP 和端口 → 尝试 TCP 连接。2. 连接成功 → 设备出现在列表中。3. 格式错误或连接失败给出提示。 |
| 禁止 | 不要修改 UDP 发现逻辑。 |

---

## 第 3 阶段：文本消息

### 任务 009 — TCP 消息通道

| 项 | 内容 |
|---|---|
| 目标 | 建立 TCP 服务端和客户端连接，收发 JSON 消息 |
| 涉及文件 | `src/network/TcpServer.*`、`src/network/TcpConnection.*`、`src/network/Protocol.*`、`src/core/SignalService.*` |
| 验收标准 | 1. 启动时监听配置端口。2. 连接其他设备时发送 message.text → 对方收到并回复 message.ack。3. 消息按 `[4字节长度][JSON]` 格式收发。4. 连接断开时通知 SignalService。 |
| 禁止 | 不要实现聊天 UI 存储。 |

### 任务 010 — 聊天窗口与消息存储

| 项 | 内容 |
|---|---|
| 目标 | 完整的单聊文本消息闭环 |
| 涉及文件 | `src/ui/chat/ChatPage.*`（完善）、`src/storage/MessageRepository.*`、`src/storage/ConversationRepository.*` |
| 验收标准 | 1. 聊天窗口显示消息气泡（自己靠右蓝色、对方靠左灰色）。2. 消息写入 SQLite，重开窗口历史可见。3. 消息状态显示（发送中→成功→失败）。4. 会话列表显示最后消息摘要和时间。 |
| 禁止 | 不要实现文件传输。 |

---

## 第 4 阶段：文件传输

### 任务 011 — 文件传输核心

| 项 | 内容 |
|---|---|
| 目标 | 实现文件传输请求、接受/拒绝、分块传输 |
| 涉及文件 | `src/core/CourierService.*`、`src/storage/TransferRepository.*` |
| 验收标准 | 1. 发送 transfer.request → 对方弹出接收/拒绝。2. 接受后分块传输（8MB/块）。3. 传输完成后校验 SHA-256。4. 进度通过 signal 实时上报。5. 传输记录写入 SQLite。 |
| 禁止 | 不要实现断点续传（P1）。 |

### 任务 012 — 传输中心页

| 项 | 内容 |
|---|---|
| 目标 | 传输中心面板展示所有传输任务 |
| 涉及文件 | `src/ui/transfer_center/TransferCenterPage.*` |
| 验收标准 | 1. 显示全部传输记录。2. 进行中任务显示进度条。3. 已完成任务可点击打开目录。4. 失败任务显示原因。5. 支持按状态过滤。 |
| 禁止 | — |

---

## 第 5 阶段：共享文件夹

### 任务 013 — 共享目录管理

| 项 | 内容 |
|---|---|
| 目标 | 添加/删除/启停共享目录 |
| 涉及文件 | `src/core/ShareService.*`、`src/storage/ShareRepository.*` |
| 验收标准 | 1. "我的共享"页可添加目录。2. 启用后对外广播 share_enabled=true。3. 可禁用或删除共享。 |
| 禁止 | 不要实现远程浏览或下载。 |

### 任务 014 — 远程浏览与下载

| 项 | 内容 |
|---|---|
| 目标 | 浏览他人共享目录并下载文件 |
| 涉及文件 | `src/ui/share/ShareBrowsePage.*`、`src/network/` 共享协议处理 |
| 验收标准 | 1. 共享文件页列出所有在线共享源。2. 点击进入 → 浏览目录。3. 点击文件 → 下载到默认目录。4. 下载记录写入 download_logs。 |
| 禁止 | — |

### 任务 015 — 共享访问授权

| 项 | 内容 |
|---|---|
| 目标 | 首次访问共享目录时弹出授权确认 |
| 涉及文件 | `src/ui/share/` 新增 AuthDialog |
| 验收标准 | 1. 他人首次访问时本机弹出授权弹窗。2. 允许/拒绝正常生效。3. "记住选择"生效。 |
| 禁止 | — |

---

## 第 6 阶段：设置与诊断

### 任务 016 — 设置页

| 项 | 内容 |
|---|---|
| 目标 | 常规和网络设置 |
| 涉及文件 | `src/ui/settings/SettingsPage.*` |
| 验收标准 | 1. 可编辑昵称、下载目录。2. 可开关发现、修改端口。3. 修改即时生效。 |
| 禁止 | — |

### 任务 017 — 网络诊断页

| 项 | 内容 |
|---|---|
| 目标 | 自助网络诊断工具 |
| 涉及文件 | `src/ui/diagnostics/DiagnosticsPage.*`、`src/core/DiagnosticsService.*` |
| 验收标准 | 1. 显示本机 IP、端口、防火墙状态。2. UDP 广播测试。3. 在线设备数。4. 每项有通过/失败标识。 |
| 禁止 | — |

---

## 第 7 阶段：打包与测试

### 任务 018 — 四平台打包

| 项 | 内容 |
|---|---|
| 目标 | 生成 Windows/Linux/macOS 安装包 |
| 涉及文件 | `CMakeLists.txt`（CPack 配置） |
| 验收标准 | 1. Windows: NSIS 安装包可安装运行。2. Linux: deb 包可在 Ubuntu 安装。3. macOS: dmg 可打开运行。4. openKylin 上 deb 包正常。 |
| 禁止 | — |
