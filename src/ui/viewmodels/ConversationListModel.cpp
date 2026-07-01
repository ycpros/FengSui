// ConversationListModel.cpp

#include "ui/viewmodels/ConversationListModel.h"

namespace FengSui {

ConversationListModel::ConversationListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ConversationListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ConversationListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case ConversationIdRole: return r.conv.conversationId;
    case PeerIdRole:         return r.conv.peerId;
    case DisplayNameRole:    return r.displayName;
    case LastMessageRole:    return r.conv.lastMessage;
    case LastMessageAtRole:  return r.conv.lastMessageAt;
    case UnreadCountRole:    return r.conv.unreadCount;
    case OnlineRole:         return r.online;
    default:                 return {};
    }
}

QHash<int, QByteArray> ConversationListModel::roleNames() const
{
    return {
        { ConversationIdRole, "conversationId" },
        { PeerIdRole,         "peerId" },
        { DisplayNameRole,    "displayName" },
        { LastMessageRole,    "lastMessage" },
        { LastMessageAtRole,  "lastMessageAt" },
        { UnreadCountRole,    "unreadCount" },
        { OnlineRole,         "online" }
    };
}

void ConversationListModel::rebuildIndex()
{
    m_indexById.clear();
    for (int i = 0; i < m_rows.size(); ++i) {
        m_indexById.insert(m_rows.at(i).conv.conversationId, i);
    }
}

void ConversationListModel::resetRows(const QList<Row>& rows)
{
    beginResetModel();
    m_rows = rows;
    rebuildIndex();
    endResetModel();
    emit countChanged();
}

void ConversationListModel::upsert(const Row& row)
{
    const auto it = m_indexById.constFind(row.conv.conversationId);
    if (it != m_indexById.constEnd()) {
        // 幂等更新：原地替换，只发变化行 dataChanged
        const int r = it.value();
        m_rows[r] = row;
        const QModelIndex idx = index(r);
        emit dataChanged(idx, idx);
        return;
    }
    // 新会话置顶
    beginInsertRows(QModelIndex(), 0, 0);
    m_rows.prepend(row);
    rebuildIndex();
    endInsertRows();
    emit countChanged();
}

void ConversationListModel::setOnline(const QString& peerId, bool online)
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).conv.peerId == peerId && m_rows.at(i).online != online) {
            m_rows[i].online = online;
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, { OnlineRole });
        }
    }
}

QString ConversationListModel::peerIdAt(int row) const
{
    if (row < 0 || row >= m_rows.size()) return {};
    return m_rows.at(row).conv.peerId;
}

QString ConversationListModel::conversationIdAt(int row) const
{
    if (row < 0 || row >= m_rows.size()) return {};
    return m_rows.at(row).conv.conversationId;
}

} // namespace FengSui
