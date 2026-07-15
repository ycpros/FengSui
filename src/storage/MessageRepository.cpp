// MessageRepository.cpp
// 消息存储仓库实现。

#include "storage/MessageRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "storage/Database.h"

namespace FengSui {

MessageRepository::MessageRepository(Database* database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

MessageSaveResult MessageRepository::saveMessage(const Message& message)
{
    if (!m_database) {
        qWarning() << "MessageRepository has no database";
        return MessageSaveResult::Failed;
    }

    QSqlQuery query(m_database->database());
    // 使用 INSERT OR IGNORE 防止重复 messageId（网络重传等边界情况）
    query.prepare("INSERT OR IGNORE INTO messages "
                  "(message_id, conversation_id, sender_id, type, content, "
                  " status, created_at, file_name, file_size, transfer_id) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(message.messageId);
    query.addBindValue(message.conversationId);
    query.addBindValue(message.senderId);
    query.addBindValue(typeToString(message.type));
    query.addBindValue(message.content);
    query.addBindValue(statusToString(message.status));
    query.addBindValue(message.createdAt.toString(Qt::ISODate));
    query.addBindValue(message.fileName);
    query.addBindValue(message.fileSize);
    query.addBindValue(message.transferId);

    if (!query.exec()) {
        qWarning() << "Failed to save message:" << query.lastError().text();
        return MessageSaveResult::Failed;
    }

    // numRowsAffected == 0 表示已有同 messageId 的记录被忽略（去重生效）
    if (query.numRowsAffected() == 0) {
        qInfo() << "Message" << message.messageId << "already exists, ignored (dedup)";
        return MessageSaveResult::Existed;
    }

    return MessageSaveResult::Inserted;
}

QList<Message> MessageRepository::getMessages(const QString& conversationId,
                                              int limit,
                                              int offset) const
{
    QList<Message> result;

    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT message_id, conversation_id, sender_id, type, content, "
                  "status, created_at, file_name, file_size, transfer_id "
                  "FROM messages "
                  "WHERE conversation_id = ? "
                  "ORDER BY created_at ASC "
                  "LIMIT ? OFFSET ?");
    query.addBindValue(conversationId);
    query.addBindValue(limit);
    query.addBindValue(offset);

    if (!query.exec()) {
        qWarning() << "Failed to get messages:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(messageFromQuery(query));
    }

    return result;
}

bool MessageRepository::updateMessageStatus(const QString& messageId,
                                            MessageStatus status)
{
    if (!m_database) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("UPDATE messages SET status = ? WHERE message_id = ?");
    query.addBindValue(statusToString(status));
    query.addBindValue(messageId);

    if (!query.exec()) {
        qWarning() << "Failed to update message status:" << query.lastError().text();
        return false;
    }

    return true;
}

std::optional<Message> MessageRepository::getMessage(const QString& messageId) const
{
    if (!m_database) {
        return std::nullopt;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT message_id, conversation_id, sender_id, type, content, "
                  "status, created_at, file_name, file_size, transfer_id "
                  "FROM messages WHERE message_id = ?");
    query.addBindValue(messageId);

    if (!query.exec()) {
        qWarning() << "Failed to get message:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return messageFromQuery(query);
}

int MessageRepository::getMessageCount(const QString& conversationId) const
{
    if (!m_database) {
        return 0;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT COUNT(*) FROM messages WHERE conversation_id = ?");
    query.addBindValue(conversationId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Failed to get message count:" << query.lastError().text();
        return 0;
    }

    return query.value(0).toInt();
}

// ---- Private Helpers ----

MessageStatus MessageRepository::statusFromString(const QString& s)
{
    if (s == QStringLiteral("sending"))   return MessageStatus::Sending;
    if (s == QStringLiteral("sent"))      return MessageStatus::Sent;
    if (s == QStringLiteral("delivered")) return MessageStatus::Delivered;
    if (s == QStringLiteral("failed"))    return MessageStatus::Failed;
    return MessageStatus::Sending;
}

QString MessageRepository::statusToString(MessageStatus s)
{
    switch (s) {
    case MessageStatus::Sending:   return QStringLiteral("sending");
    case MessageStatus::Sent:      return QStringLiteral("sent");
    case MessageStatus::Delivered: return QStringLiteral("delivered");
    case MessageStatus::Failed:    return QStringLiteral("failed");
    }
    return QStringLiteral("sending");
}

MessageType MessageRepository::typeFromString(const QString& s)
{
    if (s == QStringLiteral("text"))         return MessageType::Text;
    if (s == QStringLiteral("file_request")) return MessageType::FileRequest;
    if (s == QStringLiteral("system"))       return MessageType::System;
    return MessageType::Text;
}

QString MessageRepository::typeToString(MessageType t)
{
    switch (t) {
    case MessageType::Text:        return QStringLiteral("text");
    case MessageType::FileRequest: return QStringLiteral("file_request");
    case MessageType::System:      return QStringLiteral("system");
    }
    return QStringLiteral("text");
}

Message MessageRepository::messageFromQuery(const QSqlQuery& query)
{
    Message msg;
    msg.messageId = query.value(0).toString();
    msg.conversationId = query.value(1).toString();
    msg.senderId = query.value(2).toString();
    msg.type = typeFromString(query.value(3).toString());
    msg.content = query.value(4).toString();
    msg.status = statusFromString(query.value(5).toString());
    msg.createdAt = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
    msg.fileName = query.value(7).toString();
    msg.fileSize = query.value(8).toLongLong();
    msg.transferId = query.value(9).toString();
    return msg;
}

} // namespace FengSui
