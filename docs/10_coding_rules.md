# 10 — 代码规范与工程约束

## 语言与编译器

- **C++ 标准**：C++17
- **编译器**：MSVC 2022+ (Windows)、GCC 11+ (Linux)、Clang 14+ (macOS)
- **编码**：UTF-8，源文件带 BOM（MSVC 兼容）
- **行尾**：LF（Linux/macOS）/ CRLF（Windows），`.gitattributes` 自动转换

## Qt 约束

- **版本**：Qt 6.8+
- **UI 技术**：Qt Widgets  ONLY。禁止 QML / Qt Quick
- **模块**：Qt6::Widgets、Qt6::Network、Qt6::Sql。按需引入 Qt6::Core（自动）
- **禁止**：不使用 Qt WebEngine、Qt WebChannel、Qt Multimedia

## 命名规范

| 类型 | 规范 | 示例 |
|---|---|---|
| 命名空间 | PascalCase | `namespace FengSui { }` |
| 类名 | PascalCase | `BeaconService`、`MainWindow` |
| 函数/方法 | camelCase | `sendMessage()`、`onPeerOnline()` |
| 成员变量 | camelCase，前缀 `m_` | `m_peerId`、`m_isOnline` |
| 局部变量 | camelCase | `peerCount`、`fileName` |
| 常量 | kPascalCase 或 UPPER_SNAKE_CASE | `kDefaultPort`、`MAX_CHUNK_SIZE` |
| 文件名 | PascalCase（与类名一致） | `BeaconService.h`、`MainWindow.cpp` |
| Signal | `on + 过去式` 或 `名词 + Changed` | `peerOnline(PeerInfo)`、`transferProgress(qint64,qint64)` |
| Slot | `on + 描述` | `onSendClicked()`、`onMessageReceived(Message)` |

## 目录规范

- 头文件使用 `#pragma once`（不用 include guards）
- `#include` 顺序：自身头文件 → Qt 头文件 → 项目内部头文件 → 标准库
- 内部 include 使用项目相对路径：`#include "core/BeaconService.h"`

## 错误处理

```cpp
// 必须：每个可能失败的操作返回错误信息
bool saveToDatabase(const Message& msg, QString& errorOut);

// 禁止：空 catch 块
try { ... } catch (...) { /* 空的❌ */ }

// 禁止：忽略返回值
someOperation(); // 如果返回 bool，必须检查❌

// 正确：
if (!someOperation()) {
    qWarning() << "Operation failed";
    return;
}
```

## 日志规范

```cpp
#include "app/Logger.h"

qDebug() << "详细调试信息，留给开发者";       // 默认不输出到文件
qInfo()  << "关键流程节点";                   // 如"Beacon started on port 8788"
qWarning() << "非致命异常";                   // 如"Heartbeat timeout for peer xxx"
qCritical() << "需要关注的错误";              // 如"Failed to initialize database"
```

## 线程规范

```cpp
// ✅ 允许：UI 线程调用 Service 的异步方法（内部用 QtConcurrent）
// ✅ 允许：Worker 线程通过 signal/slot 返回结果（Qt::QueuedConnection 自动）

// ❌ 禁止：在 UI 线程执行阻塞 IO
void MainWindow::onSendClicked() {
    file.readAll();  // 可能阻塞 UI ❌
}

// ✅ 正确：
QtConcurrent::run([this, path]() {
    auto data = readFile(path);
    emit fileReadCompleted(data);
});
```

## 禁止项清单

| 类别 | 禁止事项 |
|---|---|
| 安全 | 默认共享 C:\\ 或 /home 根目录 |
| 安全 | 默认监听 0.0.0.0（应绑定具体网卡） |
| 安全 | 明文存储密码或密钥 |
| 安全 | 接受来自任何 IP 的连接而不做任何校验 |
| 工程 | 硬编码文件路径（必须用 AppSettings） |
| 工程 | 裸指针管理资源（用 QScopedPointer / std::unique_ptr / QObject parent） |
| 工程 | 在头文件中 `using namespace` |
| 工程 | 引入 OpenSSL / TLS（V1.0 不用） |
| 工程 | 引入任何 web 前端框架 |
| 工程 | 重写项目目录结构（按 [04_architecture.md](./04_architecture.md) 执行） |
| 工程 | 随意引入第三方库 |
| 代码 | UI 层直接访问 SQLite |
| 代码 | Network 层直接操作 QWidget |
| 代码 | 扩展现有任务的功能范围（严格按 [09_task_breakdown.md](./09_task_breakdown.md)） |
| 代码 | 写临时代码或 TODO 注释而不解决 |
