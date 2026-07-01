// MessageListModel.h
// 消息列表模型：承载当前会话的消息，按 messageId 更新状态（增量），追加新消息。
// isOutgoing 由 senderId==localPeerId 计算。

#pragma once

#include "models/Message.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>

namespace FengSui {

class MessageListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY countChanged)

public:
    enum Roles {
        MessageIdRole = Qt::UserRole + 1,
        SenderIdRole,
        IsOutgoingRole,
        TypeRole,
        ContentRole,
        CreatedAtRole,
        StatusRole,
        FileNameRole,
        FileSizeRole,
        TransferIdRole
    };

    explicit MessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const { return m_rows.size(); }

    void setLocalPeerId(const QString& id) { m_localPeerId = id; }

    void resetFrom(const QList<Message>& messages);
    void append(const Message& msg);                          // 幂等：已存在则整行更新
    void updateStatus(const QString& messageId, int status);  // 只发 StatusRole
    bool contains(const QString& messageId) const { return m_indexById.contains(messageId); }

signals:
    void countChanged();

private:
    QList<Message> m_rows;
    QHash<QString, int> m_indexById;
    QString m_localPeerId;
    void rebuildIndex();
};

} // namespace FengSui
