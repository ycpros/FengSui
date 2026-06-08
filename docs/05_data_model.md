# 05 — 数据模型

> 所有结构体定义在 `src/models/` 下，纯头文件，不依赖 UI 层。

---

## PeerInfo — 设备/联系人

```cpp
// models/PeerInfo.h
#pragma once
#include <QString>
#include <QDateTime>

struct PeerInfo {
    QString peerId;          // 唯一标识，SHA-256(主机名+MAC+随机盐)，如 "a1b2c3d4"
    QString displayName;     // 用户设置的昵称，如 "Lily-PC"
    QString deviceName;      // 系统主机名，如 "DESKTOP-001"
    QString ip;              // IPv4 地址，如 "192.168.1.20"
    quint16 port = 0;        // TCP 监听端口，默认 8787
    QString os;              // 操作系统，"windows" / "linux" / "macos"
    bool online = false;     // 当前是否在线
    QDateTime lastSeenAt;    // 最后一次在线时间
    bool shareEnabled = false; // 是否启用了共享目录
    QString version;         // 客户端版本号，如 "0.1.0"
};
```

## Message — 消息

```cpp
// models/Message.h
#pragma once
#include <QString>
#include <QDateTime>

enum class MessageType {
    Text,           // 文本消息
    FileRequest,    // 文件传输请求（聊天窗口显示为"发送了 xxx.pdf"）
    System          // 系统消息（如"对方已接收文件"）
};

enum class MessageStatus {
    Sending,
    Sent,
    Delivered,
    Failed
};

struct Message {
    QString messageId;       // UUID，如 "msg_a1b2c3d4"
    QString conversationId;  // 所属会话 ID
    QString senderId;        // 发送方 peerId
    MessageType type = MessageType::Text;
    QString content;         // 文本内容或文件元信息 JSON
    QDateTime createdAt;     // 本地创建时间
    MessageStatus status = MessageStatus::Sending;
    // 如果是文件请求，额外字段：
    QString fileName;
    qint64 fileSize = 0;
    QString transferId;      // 关联的 TransferTask::transferId
};
```

## Conversation — 会话

```cpp
// models/Conversation.h
#pragma once
#include <QString>
#include <QDateTime>

struct Conversation {
    QString conversationId;  // "conv_{peerId1}_{peerId2}" 两个 peerId 排序后拼接
    QString peerId;          // 对方的 peerId
    QString lastMessage;     // 最后一条消息摘要（用于列表展示）
    QDateTime lastMessageAt; // 最后消息时间
    int unreadCount = 0;     // 未读计数
};
```

## TransferTask — 文件传输任务

```cpp
// models/TransferTask.h
#pragma once
#include <QString>
#include <QDateTime>

enum class TransferDirection {
    Upload,     // 我发出
    Download    // 我接收
};

enum class TransferStatus {
    Waiting,    // 等待对方接受
    Rejected,   // 对方拒绝
    Transferring,
    Completed,
    Failed,
    Cancelled
};

struct TransferTask {
    QString transferId;          // UUID
    TransferDirection direction;
    QString peerId;              // 对方 peerId
    QString fileName;
    QString filePath;            // 本地文件路径（发出为源，接收为下载目标）
    qint64 fileSize = 0;
    qint64 transferredBytes = 0; // 已传输字节
    QString sha256;              // 文件校验
    TransferStatus status = TransferStatus::Waiting;
    QString errorMessage;        // 失败原因
    QDateTime createdAt;
    QDateTime completedAt;
};
```

## SharedFolder — 共享目录

```cpp
// models/SharedFolder.h
#pragma once
#include <QString>

struct SharedFolder {
    QString shareId;       // UUID
    QString localPath;     // 本地绝对路径，如 "D:/Projects/design"
    QString displayName;   // 对外显示名称，默认取目录名
    bool isActive = true;  // 是否启用
};
```

## AccessGrant — 共享访问授权

```cpp
// models/AccessGrant.h
#pragma once
#include <QString>
#include <QDateTime>

struct AccessGrant {
    QString peerId;           // 被授权设备的 peerId
    QString shareId;          // 被授权的共享目录 shareId
    QDateTime grantedAt;
    bool remember = false;    // 是否记住选择
};
```

## DownloadLog — 下载记录

```cpp
// models/DownloadLog.h
#pragma once
#include <QString>
#include <QDateTime>

struct DownloadLog {
    QString logId;       // UUID
    QString peerId;      // 来源设备 peerId
    QString shareId;     // 来源共享目录 shareId
    QString remotePath;  // 源文件远程路径
    QString localPath;   // 本地保存路径
    QString fileName;
    qint64 fileSize = 0;
    QDateTime downloadedAt;
    bool success = false;
};
```

## AppSettings — 应用设置

```cpp
// models/AppSettings.h
#pragma once
#include <QString>

struct AppSettings {
    QString displayName;           // 用户昵称
    QString downloadDir;           // 默认下载目录
    bool discoveryEnabled = true;  // 是否启用局域网发现
    quint16 listenPort = 8787;     // TCP 监听端口
    bool onboardingCompleted = false; // 是否已完成首次向导
    bool minimizeToTray = true;    // 关闭后最小化到托盘
};
```
