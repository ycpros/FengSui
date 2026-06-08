# 12 — Agent 实施说明

> 本文档是给 AI Coding Agent 的行为准则。每次任务前请先阅读本文件。

## 技术边界

| 类别 | 允许 | 禁止 |
|---|---|---|
| 语言 | C++17 | C++20 特性谨慎（视编译器支持） |
| UI | Qt 6 Widgets | QML、Qt Quick、Qt WebEngine |
| 构建 | CMake 3.20+ | qmake、MSBuild |
| 网络 | Qt Network (QUdpSocket, QTcpSocket) | Boost.Asio、Standalone Asio、任何第三方网络库 |
| 数据库 | SQLite (QSqlDatabase) | MySQL、PostgreSQL、任何需独立服务的数据库 |
| 序列化 | QJsonDocument | Protobuf、MessagePack |
| 架构 | 单进程 | IPC、多进程、后台服务 daemon |
| 加密 | —（V1.0 无） | OpenSSL、任何 TLS 库 |
| 平台 | Windows、Ubuntu、macOS、openKylin | iOS、Android |

## 项目文档引用

在开始任何实现之前，必须阅读：
1. [01_project_brief.md](./01_project_brief.md) — 项目全貌
2. [03_mvp_scope.md](./03_mvp_scope.md) — 具体做什么
3. [04_architecture.md](./04_architecture.md) — 怎么做
4. [05_data_model.md](./05_data_model.md) — 数据结构
5. [06_protocol.md](./06_protocol.md) — 通信协议
6. [07_database_schema.md](./07_database_schema.md) — 数据库
7. [08_ui_spec.md](./08_ui_spec.md) — 交互行为
8. [09_task_breakdown.md](./09_task_breakdown.md) — 当前任务
9. [10_coding_rules.md](./10_coding_rules.md) — 怎么写

## 分层禁止规则

```
UI 层：不得 #include network/ 和 storage/（除了 models/）
       不得执行阻塞 IO
       不得直接拼 SQL

Core 层：不得 #include ui/ 和 QWidget
        通过 signal/slot 通知 UI 状态变化

Network 层：不得 #include ui/ 和 QWidget
           不得操作数据库

Storage 层：不得 #include ui/ 和 network/
           SQLite 是唯一数据出口
```

## 实施方式

1. **每次只完成一个任务**（按 09_task_breakdown.md 顺序）
2. **每个任务完成后必须交付：**
   - 修改了哪些文件（完整路径）
   - 实现了什么功能
   - 如何测试（给出具体操作步骤）
   - 是否影响已有功能（回归检查）
3. **不得擅自扩大功能范围**。如果发现任务描述不足以实现，请先提出疑问，不要自行补充"可能需要的"功能。
4. **如果遇到技术选择歧义**，优先按本文档的技术边界表执行，其次参考 04_architecture.md 的分层规则。

## 强制禁止事项

- 不得重写项目目录结构
- 不得引入 10_coding_rules.md 禁止项清单外的依赖
- 不得将临时代码（// TODO / // FIXME / 注释掉的代码）写入主流程
- 不得跳过错误处理
- 不得默认共享用户家目录或根目录
- 不得在 UI 线程执行大文件 IO
- 不得忽略编译警告
- 不得写出空 catch 块
- 不得在头文件中 using namespace

## 注释规范

所有代码必须使用**中文注释**，注释风格遵循 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Comments) 的注释规则：

- **文件头注释**：每个 `.h` / `.cpp` 文件顶部包含文件用途说明
  ```cpp
  // BeaconService.h
  // 局域网设备自动发现服务，基于 UDP 组播实现节点上线/下线/心跳检测。
  // 不依赖 Avahi / Bonjour 等系统 mDNS 守护进程。
  ```

- **类注释**：每个类声明前用 `//` 描述职责和关键设计决策
  ```cpp
  // BeaconService 负责 UDP 组播发现与心跳维护。
  // 不直接操作 UI，状态变更通过 signal 通知外部。
  // 线程安全：内部使用 QTimer，回调在调用线程执行。
  class BeaconService : public QObject { ... };
  ```

- **函数注释**：公开函数声明前注释功能、参数含义、返回值、线程安全性
  ```cpp
  // 向指定对端发送文本消息。
  // peerId: 目标设备 peerId，不能为空。
  // text: 消息正文，UTF-8 编码。
  // 返回 true 表示消息已进入发送队列（不保证送达）。
  bool sendMessage(const QString& peerId, const QString& text);
  ```

- **私有函数注释**：`.cpp` 中所有私有成员函数和文件级静态函数声明前须有 `//` 注释，说明用途和关键前置条件
  ```cpp
  // 处理收到的 presence.hello 消息：提取对端信息，更新 m_peers 缓存，
  // 若为新对等体则发射 peerOnline 信号，否则仅刷新心跳时间戳。
  // json: 已通过 Protocol::unpack() 解析的 JSON 对象，必须包含 type/peer_id 字段。
  void BeaconService::handleHello(const QJsonObject& json) { ... }
  ```

- **复杂逻辑块注释**：超过 15 行的连续逻辑块须用 `//` 分段注释，说明每步做什么。
  若逻辑对应设计文档中的协议步骤，须引用文档编号（如 `// 步骤 2: 06_protocol.md §3.1`）
  ```cpp
  // 步骤 1：检查读缓冲中是否有完整帧（4 字节长度 + 载荷）
  while (Protocol::extractFrame(m_readBuffer, frame, error)) {
      // 步骤 2：将帧载荷解析为 JSON 对象
      QJsonObject msg = Protocol::unpack(frame);
      // 步骤 3：通知上层（SignalService）处理完整消息
      emit messageReceived(msg);
  }
  // 步骤 4：extractFrame 返回 false 且 error 为空 → 数据不完整，等待下次 readyRead
  ```

- **错误处理注释**：每个非常规错误处理路径须用行内 `//` 说明触发条件与恢复策略
  ```cpp
  if (!m_socket->waitForConnected(3000)) {
      // 连接超时：局域网内 3 秒足以完成 TCP 握手，超时通常意味着目标端口未监听或防火墙拦截。
      // 恢复策略：关闭 socket 并通知上层连接失败，由 SignalService 负责重试或提示用户。
      m_socket->close();
      emit errorOccurred(QStringLiteral("连接超时: %1").arg(m_socket->errorString()));
      return;
  }
  ```

- **Qt connect 注释**：每个 `connect()` 调用须用 `//` 注释说明信号传递方向和数据流意图
  ```cpp
  // UdpDiscovery → BeaconService: 原始 UDP 数据报到达，交由业务层解析
  connect(m_discovery, &UdpDiscovery::datagramReceived,
          this, &BeaconService::onDatagramReceived);

  // TcpConnection → SignalService: 完整 JSON 消息到达，进入消息路由
  connect(conn, &TcpConnection::messageReceived,
          this, &SignalService::onMessageReceived);

  // SignalService → UI: 通知聊天页面有新消息到达
  connect(this, &SignalService::messageReceived,
          chatPage, &ChatPage::onMessageReceived);
  ```

- **变量注释**：成员变量、静态常量、关键局部变量须用 `//` 注释说明含义和约束

  | 变量类型 | 注释要求 | 位置 |
  |---|---|---|
  | 成员变量 | 必须注释，说明含义及约束（单位、范围、所有权） | 头文件声明处 |
  | 静态/文件级常量 | 必须注释，说明取值理由和单位 | 声明处 |
  | 关键局部变量 | 变量名不完全自明时须注释 | 定义处行内 |

  ```cpp
  // .h 成员变量示例
  class BeaconService : public QObject {
  private:
      QHash<QString, PeerInfo> m_peers;   // peerId → 对等体信息映射，含在线和离线设备
      QTimer* m_helloTimer = nullptr;     // hello 广播定时器，间隔 5 秒（06_protocol.md）
      int m_helloIntervalMs = 5000;       // 广播间隔（毫秒），任务 006 验收标准要求 5 秒
      static constexpr int kPeerTimeoutMs = 15000;  // 对等体超时（毫秒），15 秒未收到心跳视为离线
  };
  ```

  ```cpp
  // .cpp 关键局部变量示例
  // ❌ 禁止：英文或无语境
  int size = 0;

  // ✅ 正确：中文说明含义
  int onlineCount = 0;  // 当前在线对等体数量，遍历 m_peers 后累加，用于更新托盘图标角标
  ```

- **行内注释**：非自明的代码行用行内 `//` 解释为什么这么做，而不是复述代码做了什么
  ```cpp
  // ❌ 禁止：复述代码
  int count = peers.size();  // 获取 peers 数量

  // ✅ 正确：解释意图
  int count = peers.size();  // 不包括标记为离线的设备，离线设备在 TTL 到期后才清理
  ```

- **TODO 注释**：使用 `// TODO(用户名): 描述` 格式，标注负责人和到期时间
  ```cpp
  // TODO(dev): 2026-Q3 替换为 TLS 加密传输
  ```

- **注释禁止事项**：
  - 不得出现无注释的公开 API（类、函数、枚举）
  - 不得出现无注释的私有函数（`.cpp` 中所有函数均须有注释）
  - 不得出现无注释的成员变量和静态常量
  - 不得出现无注释的 `connect()` 调用
  - 不得使用英文注释（全文一致用中文）
  - 不得出现被注释掉的代码块
  - 不得使用 `/* */` 多行注释（统一用 `//`）
  - 不得提交超过 15 行无分段注释的连续逻辑块
