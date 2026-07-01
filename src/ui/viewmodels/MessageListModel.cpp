// MessageListModel.cpp

#include "ui/viewmodels/MessageListModel.h"

namespace FengSui {

MessageListModel::MessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int MessageListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant MessageListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Message& m = m_rows.at(index.row());
    switch (role) {
    case MessageIdRole:  return m.messageId;
    case SenderIdRole:   return m.senderId;
    case IsOutgoingRole: return m.senderId == m_localPeerId;
    case TypeRole:       return static_cast<int>(m.type);
    case ContentRole:    return m.content;
    case CreatedAtRole:  return m.createdAt;
    case StatusRole:     return static_cast<int>(m.status);
    case FileNameRole:   return m.fileName;
    case FileSizeRole:   return m.fileSize;
    case TransferIdRole: return m.transferId;
    default:             return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        { MessageIdRole,  "messageId" },
        { SenderIdRole,   "senderId" },
        { IsOutgoingRole, "isOutgoing" },
        { TypeRole,       "type" },
        { ContentRole,    "content" },
        { CreatedAtRole,  "createdAt" },
        { StatusRole,     "status" },
        { FileNameRole,   "fileName" },
        { FileSizeRole,   "fileSize" },
        { TransferIdRole, "transferId" }
    };
}

void MessageListModel::rebuildIndex()
{
    m_indexById.clear();
    for (int i = 0; i < m_rows.size(); ++i) {
        m_indexById.insert(m_rows.at(i).messageId, i);
    }
}

void MessageListModel::resetFrom(const QList<Message>& messages)
{
    beginResetModel();
    m_rows = messages;
    rebuildIndex();
    endResetModel();
    emit countChanged();
}

void MessageListModel::append(const Message& msg)
{
    const auto it = m_indexById.constFind(msg.messageId);
    if (it != m_indexById.constEnd()) {
        // 幂等：已存在则整行更新（避免 messageStored 重复触发导致重复气泡）
        const int r = it.value();
        m_rows[r] = msg;
        const QModelIndex idx = index(r);
        emit dataChanged(idx, idx);
        return;
    }
    const int row = m_rows.size();
    beginInsertRows(QModelIndex(), row, row);
    m_rows.append(msg);
    m_indexById.insert(msg.messageId, row);
    endInsertRows();
    emit countChanged();
}

void MessageListModel::updateStatus(const QString& messageId, int status)
{
    const auto it = m_indexById.constFind(messageId);
    if (it == m_indexById.constEnd()) {
        return;
    }
    const int r = it.value();
    m_rows[r].status = static_cast<MessageStatus>(status);
    const QModelIndex idx = index(r);
    emit dataChanged(idx, idx, { StatusRole });
}

} // namespace FengSui
