// Conversation.h
// 会话数据结构：记录与某个设备的一对一聊天会话元信息。

#pragma once

#include <QObject>       // Q_GADGET / Q_PROPERTY 需要
#include <QString>
#include <QDateTime>

namespace FengSui {

// 会话数据结构。
// conversationId 由两个 peerId 排序后拼接生成，确保双向唯一。
struct Conversation {
    Q_GADGET
    Q_PROPERTY(QString conversationId MEMBER conversationId)
    Q_PROPERTY(QString peerId MEMBER peerId)
    Q_PROPERTY(QString lastMessage MEMBER lastMessage)
    Q_PROPERTY(QDateTime lastMessageAt MEMBER lastMessageAt)
    Q_PROPERTY(int unreadCount MEMBER unreadCount)
public:
    QString conversationId;  // "conv_{peerId1}_{peerId2}"，两个 peerId 排序后拼接
    QString peerId;          // 对方的 peerId
    QString lastMessage;     // 最后一条消息摘要（用于列表展示）
    QDateTime lastMessageAt; // 最后消息时间
    int unreadCount = 0;     // 未读计数
};

} // namespace FengSui

// 注册元类型，使 Conversation 可通过 Qt signal/slot 传递
Q_DECLARE_METATYPE(FengSui::Conversation)
