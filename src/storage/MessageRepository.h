// MessageRepository.h
// 消息存储仓库：管理 messages 表 CRUD，支持历史查询和状态更新。

#pragma once

#include "models/Message.h"

#include <QObject>
#include <QString>
#include <QList>
#include <optional>

class QSqlQuery;

namespace FengSui {

    // 消息保存结果情况
    enum class MessageSaveResult
    {
        Inserted,   // 首次写入成功
        Existed,    // 已有相同messageId
        Failed      // SQLite执行失败
    };

class Database;

// 消息持久化仓库。
// 负责 messages 表的增删查改，不感知网络层或 UI 层。
class MessageRepository : public QObject {
    Q_OBJECT

public:
    explicit MessageRepository(Database* database, QObject* parent = nullptr);

    // 保存一条消息（使用 INSERT OR IGNORE 防止重复 messageId）。
    // 返回 true 表示成功写入。
    MessageSaveResult saveMessage(const Message& message);

    // 分页获取会话消息历史，按 created_at 升序排列（旧消息在前）。
    // conversationId: 会话 ID。
    // limit: 单次返回最大条数，默认 50。
    // offset: 跳过前 N 条，用于翻页。
    QList<Message> getMessages(const QString& conversationId,
                               int limit = 50,
                               int offset = 0) const;

    // 更新消息投递状态。
    bool updateMessageStatus(const QString& messageId, MessageStatus status);

    // 按 messageId 查找单条消息。
    std::optional<Message> getMessage(const QString& messageId) const;

    // 获取会话中消息总数。
    int getMessageCount(const QString& conversationId) const;

private:
    // 枚举 ↔ DB 字符串互转辅助方法
    static MessageStatus statusFromString(const QString& s);
    static QString statusToString(MessageStatus s);
    static MessageType typeFromString(const QString& s);
    static QString typeToString(MessageType t);

    // 从 QSqlQuery 当前行构造 Message 结构体。
    static Message messageFromQuery(const QSqlQuery& query);

    Database* m_database = nullptr;
};

} // namespace FengSui
