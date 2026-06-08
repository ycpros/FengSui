// ConversationRepository.cpp
// 会话存储仓库实现。

#include "storage/ConversationRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "storage/Database.h"

namespace FengSui {

ConversationRepository::ConversationRepository(Database* database,
                                               const QString& localPeerId,
                                               QObject* parent)
    : QObject(parent)
    , m_database(database)
    , m_localPeerId(localPeerId)
{
}

Conversation ConversationRepository::createOrGetConversation(const QString& otherPeerId)
{
    const QString convId = generateConversationId(otherPeerId);

    if (!m_database) {
        qWarning() << "ConversationRepository has no database";
        return {};
    }

    if (otherPeerId.trimmed().isEmpty()) {
        qWarning() << "ConversationRepository cannot create conversation for empty peerId";
        return {};
    }

    if (!getPeer(otherPeerId).has_value()) {
        // 收到入站消息时可能只有 from peerId，没有完整发现信息。
        // 先写入最小对等体记录，避免 conversations 外键插入失败。
        PeerInfo peer;
        peer.peerId = otherPeerId;
        peer.displayName = otherPeerId;
        peer.deviceName = otherPeerId;
        peer.online = false;
        ensurePeer(peer);
    }

    QSqlDatabase db = m_database->database();

    // 先查询现有会话
    {
        QSqlQuery query(db);
        query.prepare("SELECT conversation_id, peer_id, last_message, "
                      "last_message_at, unread_count "
                      "FROM conversations WHERE conversation_id = ?");
        query.addBindValue(convId);

        if (query.exec() && query.next()) {
            return conversationFromQuery(query);
        }
    }

    // 不存在则插入新会话
    {
        QSqlQuery query(db);
        query.prepare("INSERT INTO conversations (conversation_id, peer_id) "
                      "VALUES (?, ?)");
        query.addBindValue(convId);
        query.addBindValue(otherPeerId);

        if (!query.exec()) {
            qWarning() << "Failed to create conversation:" << query.lastError().text();
            return {};
        }
    }

    Conversation conv;
    conv.conversationId = convId;
    conv.peerId = otherPeerId;
    return conv;
}

QList<Conversation> ConversationRepository::getAllConversations() const
{
    QList<Conversation> result;

    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    // 有消息的会话按最后消息时间降序，无消息的会话排在末尾
    if (!query.exec("SELECT conversation_id, peer_id, last_message, "
                    "last_message_at, unread_count "
                    "FROM conversations "
                    "ORDER BY last_message_at DESC")) {
        qWarning() << "Failed to get conversations:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(conversationFromQuery(query));
    }

    return result;
}

std::optional<Conversation> ConversationRepository::getConversationByPeerId(
    const QString& otherPeerId) const
{
    if (!m_database) {
        return std::nullopt;
    }

    const QString convId = generateConversationId(otherPeerId);

    QSqlQuery query(m_database->database());
    query.prepare("SELECT conversation_id, peer_id, last_message, "
                  "last_message_at, unread_count "
                  "FROM conversations WHERE conversation_id = ?");
    query.addBindValue(convId);

    if (!query.exec()) {
        qWarning() << "Failed to get conversation by peerId:"
                   << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return conversationFromQuery(query);
}

std::optional<PeerInfo> ConversationRepository::getPeer(const QString& peerId) const
{
    if (!m_database || peerId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT peer_id, display_name, device_name, ip, port, os, "
                  "online, last_seen_at, share_enabled, version "
                  "FROM peers WHERE peer_id = ?");
    query.addBindValue(peerId);

    if (!query.exec()) {
        qWarning() << "Failed to get peer:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return peerFromQuery(query);
}

std::optional<PeerInfo> ConversationRepository::getPeerForConversation(
    const QString& conversationId) const
{
    if (!m_database || conversationId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT p.peer_id, p.display_name, p.device_name, p.ip, p.port, "
                  "p.os, p.online, p.last_seen_at, p.share_enabled, p.version "
                  "FROM conversations c "
                  "JOIN peers p ON p.peer_id = c.peer_id "
                  "WHERE c.conversation_id = ?");
    query.addBindValue(conversationId);

    if (!query.exec()) {
        qWarning() << "Failed to get peer for conversation:"
                   << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return peerFromQuery(query);
}

bool ConversationRepository::updateLastMessage(const QString& conversationId,
                                               const QString& summary,
                                               const QDateTime& at)
{
    if (!m_database) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("UPDATE conversations SET last_message = ?, last_message_at = ? "
                  "WHERE conversation_id = ?");
    query.addBindValue(summary);
    query.addBindValue(at.toString(Qt::ISODate));
    query.addBindValue(conversationId);

    if (!query.exec()) {
        qWarning() << "Failed to update last message:" << query.lastError().text();
        return false;
    }

    return true;
}

bool ConversationRepository::incrementUnreadCount(const QString& conversationId)
{
    if (!m_database) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("UPDATE conversations SET unread_count = unread_count + 1 "
                  "WHERE conversation_id = ?");
    query.addBindValue(conversationId);

    if (!query.exec()) {
        qWarning() << "Failed to increment unread count:" << query.lastError().text();
        return false;
    }

    return true;
}

bool ConversationRepository::resetUnreadCount(const QString& conversationId)
{
    if (!m_database) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("UPDATE conversations SET unread_count = 0 "
                  "WHERE conversation_id = ?");
    query.addBindValue(conversationId);

    if (!query.exec()) {
        qWarning() << "Failed to reset unread count:" << query.lastError().text();
        return false;
    }

    return true;
}

bool ConversationRepository::ensurePeer(const PeerInfo& peer)
{
    if (!m_database) {
        return false;
    }

    if (peer.peerId.trimmed().isEmpty()) {
        qWarning() << "Cannot ensure peer with empty peer_id";
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("INSERT OR REPLACE INTO peers "
                  "(peer_id, display_name, device_name, ip, port, os, "
                  " online, last_seen_at, share_enabled, version) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(peer.peerId);
    query.addBindValue(peer.displayName);
    query.addBindValue(peer.deviceName);
    query.addBindValue(peer.ip);
    query.addBindValue(peer.port);
    query.addBindValue(peer.os);
    // SQLite 无原生 BOOL，用整数 0/1 存储
    query.addBindValue(peer.online ? 1 : 0);
    query.addBindValue(peer.lastSeenAt.toString(Qt::ISODate));
    query.addBindValue(peer.shareEnabled ? 1 : 0);
    query.addBindValue(peer.version);

    if (!query.exec()) {
        qWarning() << "Failed to ensure peer:" << query.lastError().text();
        return false;
    }

    return true;
}

// ---- Private Helpers ----

QString ConversationRepository::generateConversationId(const QString& otherPeerId) const
{
    // 将两个 peerId 按字典序排序后拼接，保证双向相同的 conversationId
    if (m_localPeerId < otherPeerId) {
        return QStringLiteral("conv_%1_%2").arg(m_localPeerId, otherPeerId);
    }
    return QStringLiteral("conv_%1_%2").arg(otherPeerId, m_localPeerId);
}

Conversation ConversationRepository::conversationFromQuery(const QSqlQuery& query)
{
    Conversation conv;
    conv.conversationId = query.value(0).toString();
    conv.peerId = query.value(1).toString();
    conv.lastMessage = query.value(2).toString();
    // last_message_at 可能为 NULL（新会话无消息）
    const QString lastAt = query.value(3).toString();
    if (!lastAt.isEmpty()) {
        conv.lastMessageAt = QDateTime::fromString(lastAt, Qt::ISODate);
    }
    conv.unreadCount = query.value(4).toInt();
    return conv;
}

PeerInfo ConversationRepository::peerFromQuery(const QSqlQuery& query)
{
    PeerInfo peer;
    peer.peerId = query.value(0).toString();
    peer.displayName = query.value(1).toString();
    peer.deviceName = query.value(2).toString();
    peer.ip = query.value(3).toString();
    peer.port = static_cast<quint16>(query.value(4).toUInt());
    peer.os = query.value(5).toString();
    peer.online = query.value(6).toInt() != 0;
    peer.lastSeenAt = QDateTime::fromString(query.value(7).toString(), Qt::ISODate);
    peer.shareEnabled = query.value(8).toInt() != 0;
    peer.version = query.value(9).toString();
    return peer;
}

} // namespace FengSui
