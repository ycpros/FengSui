// ConversationRepository.h
// 会话存储仓库：管理 conversations 表 CRUD 及 peers 表同步。
// 会话 ID 由两个 peerId 字典序排序后拼接生成，确保双向唯一。

#pragma once

#include "models/Conversation.h"
#include "models/PeerInfo.h"

#include <QObject>
#include <QString>
#include <QList>
#include <optional>

class QSqlQuery;

namespace FengSui {

class Database;

// 会话持久化仓库。
// 负责 conversations 表的增删查改，同时维护 peers 表的在线状态和基本信息。
class ConversationRepository : public QObject {
    Q_OBJECT

public:
    // 创建会话仓库。
    // database: 数据库管理器实例，生命周期必须长于本对象。
    // localPeerId: 本机 peer_id，用于生成确定性会话 ID。
    // parent: Qt 父对象。
    explicit ConversationRepository(Database* database,
                                    const QString& localPeerId,
                                    QObject* parent = nullptr);

    // 获取或创建与指定 peer 的会话。
    // 会话 ID 由两个 peerId 排序后拼装，双向唯一。
    // 首次创建时自动确保对等体信息录入 peers 表。
    Conversation createOrGetConversation(const QString& otherPeerId);

    // 获取所有会话，按 last_message_at 降序排列（最新消息的会话排在前面）。
    QList<Conversation> getAllConversations() const;

    // 按 peerId 查找会话；不存在时返回 nullopt。
    std::optional<Conversation> getConversationByPeerId(const QString& otherPeerId) const;

    // 按 peerId 查找对等体信息；不存在时返回 nullopt。
    std::optional<PeerInfo> getPeer(const QString& peerId) const;

    // 根据会话 ID 查找对等体信息；会话或对等体不存在时返回 nullopt。
    std::optional<PeerInfo> getPeerForConversation(const QString& conversationId) const;

    // 更新最后一条消息摘要和时间。
    // conversationId: 会话 ID。
    // summary: 消息内容摘要（截取前若干字符）。
    // at: 消息时间。
    bool updateLastMessage(const QString& conversationId,
                           const QString& summary,
                           const QDateTime& at);

    // 未读计数 +1（收到消息且用户未打开该会话时调用）。
    bool incrementUnreadCount(const QString& conversationId);

    // 未读计数归零（用户打开该会话时调用）。
    bool resetUnreadCount(const QString& conversationId);

    // 确保对等体信息存在于 peers 表（INSERT OR REPLACE）。
    // 用于在收发消息时同步对等体的基本信息。
    bool ensurePeer(const PeerInfo& peer);

private:
    // 生成 "conv_{peerId1}_{peerId2}"，其中 peerId1 < peerId2（字典序）。
    QString generateConversationId(const QString& otherPeerId) const;

    // 从 QSqlQuery 当前行构造 Conversation 结构体。
    static Conversation conversationFromQuery(const QSqlQuery& query);

    // 从 QSqlQuery 当前行构造 PeerInfo 结构体。
    static PeerInfo peerFromQuery(const QSqlQuery& query);

    Database* m_database = nullptr;
    QString m_localPeerId;
};

} // namespace FengSui
