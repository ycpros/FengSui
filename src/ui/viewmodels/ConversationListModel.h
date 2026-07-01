// ConversationListModel.h
// 会话列表模型：按 conversationId 增量 upsert，维护 id→行 哈希。
// 会话名/在线态由 ChatViewModel 解析后写入（模型本身不查服务）。

#pragma once

#include "models/Conversation.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>

namespace FengSui {

class ConversationListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY countChanged)

public:
    enum Roles {
        ConversationIdRole = Qt::UserRole + 1,
        PeerIdRole,
        DisplayNameRole,
        LastMessageRole,
        LastMessageAtRole,
        UnreadCountRole,
        OnlineRole
    };

    // 一行 = 会话 + 解析出的展示名/在线态
    struct Row {
        Conversation conv;
        QString      displayName;
        bool         online = false;
    };

    explicit ConversationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const { return m_rows.size(); }

    void resetRows(const QList<Row>& rows);
    void upsert(const Row& row);                       // 幂等：按 conversationId
    void setOnline(const QString& peerId, bool online);
    Q_INVOKABLE QString peerIdAt(int row) const;
    Q_INVOKABLE QString conversationIdAt(int row) const;

signals:
    void countChanged();

private:
    QList<Row> m_rows;
    QHash<QString, int> m_indexById;
    void rebuildIndex();
};

} // namespace FengSui
