// Message.h
// 消息数据结构：文本消息、文件传输请求、系统通知。

#pragma once

#include <QString>
#include <QDateTime>

namespace FengSui {

// 消息类型
enum class MessageType {
    Text,           // 文本消息
    FileRequest,    // 文件传输请求（聊天窗口显示为"发送了 xxx.pdf"）
    System          // 系统消息（如"对方已接收文件"）
};

// 消息投递状态
enum class MessageStatus {
    Sending,        // 正在发送
    Sent,           // 已发送（等待对方 ack）
    Delivered,      // 对方已确认收到
    Failed          // 发送失败
};

// 消息数据结构。
// 存储在 messages 表中，通过 conversationId 关联会话。
struct Message {
    QString messageId;       // UUID，如 "msg_a1b2c3d4"
    QString conversationId;  // 所属会话 ID
    QString senderId;        // 发送方 peerId
    MessageType type = MessageType::Text;
    QString content;         // 文本内容或文件元信息 JSON
    QDateTime createdAt;     // 本地创建时间
    MessageStatus status = MessageStatus::Sending;

    // 文件请求专用字段（type == FileRequest 时有效）
    QString fileName;
    qint64 fileSize = 0;
    QString transferId;      // 关联的 TransferTask::transferId
};

} // namespace FengSui

// 注册元类型，使 Message 可通过 Qt signal/slot 传递
Q_DECLARE_METATYPE(FengSui::Message)
