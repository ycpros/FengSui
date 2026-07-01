// Message.h
// 消息数据结构：文本消息、文件传输请求、系统通知。

#pragma once

#include "models/ModelEnums.h"  // MessageType / MessageStatus 枚举定义 + QML 反射注册
#include <QObject>              // Q_GADGET / Q_PROPERTY 需要
#include <QString>
#include <QDateTime>

namespace FengSui {

// 消息数据结构。
// 存储在 messages 表中，通过 conversationId 关联会话。
struct Message {
    Q_GADGET
    Q_PROPERTY(QString messageId MEMBER messageId)
    Q_PROPERTY(QString conversationId MEMBER conversationId)
    Q_PROPERTY(QString senderId MEMBER senderId)
    Q_PROPERTY(MessageType type MEMBER type)
    Q_PROPERTY(QString content MEMBER content)
    Q_PROPERTY(QDateTime createdAt MEMBER createdAt)
    Q_PROPERTY(MessageStatus status MEMBER status)
    Q_PROPERTY(QString fileName MEMBER fileName)
    Q_PROPERTY(qint64 fileSize MEMBER fileSize)
    Q_PROPERTY(QString transferId MEMBER transferId)
public:
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
