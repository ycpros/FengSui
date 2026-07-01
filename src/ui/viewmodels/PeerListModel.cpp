// PeerListModel.cpp
// 在线设备列表模型实现。

#include "ui/viewmodels/PeerListModel.h"

namespace FengSui {

PeerListModel::PeerListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int PeerListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_rows.size();
}

QVariant PeerListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const PeerInfo& p = m_rows.at(index.row());
    switch (role) {
    case PeerIdRole:       return p.peerId;
    case DisplayNameRole:  return p.displayName;
    case DeviceNameRole:   return p.deviceName;
    case IpRole:           return p.ip;
    case PortRole:         return p.port;
    case OsRole:           return p.os;
    case OnlineRole:       return p.online;
    case ShareEnabledRole: return p.shareEnabled;
    case EndpointRole:     return p.ip + QLatin1Char(':') + QString::number(p.port);
    default:               return {};
    }
}

QHash<int, QByteArray> PeerListModel::roleNames() const
{
    return {
        { PeerIdRole,       "peerId" },
        { DisplayNameRole,  "displayName" },
        { DeviceNameRole,   "deviceName" },
        { IpRole,           "ip" },
        { PortRole,         "port" },
        { OsRole,           "os" },
        { OnlineRole,       "online" },
        { ShareEnabledRole, "shareEnabled" },
        { EndpointRole,     "endpoint" }
    };
}

void PeerListModel::resetFrom(const QList<PeerInfo>& peers)
{
    beginResetModel();
    m_rows = peers;
    rebuildIndex();
    endResetModel();
    emit countChanged();
}

void PeerListModel::upsert(const PeerInfo& peer)
{
    const auto it = m_indexById.constFind(peer.peerId);
    if (it != m_indexById.constEnd()) {
        // 已存在：原地更新，只发变化行的 dataChanged
        const int row = it.value();
        m_rows[row] = peer;
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
        return;
    }
    // 新设备：追加一行
    const int row = m_rows.size();
    beginInsertRows(QModelIndex(), row, row);
    m_rows.append(peer);
    m_indexById.insert(peer.peerId, row);
    endInsertRows();
    emit countChanged();
}

void PeerListModel::remove(const QString& peerId)
{
    const auto it = m_indexById.constFind(peerId);
    if (it == m_indexById.constEnd()) {
        return;
    }
    const int row = it.value();
    beginRemoveRows(QModelIndex(), row, row);
    m_rows.removeAt(row);
    endRemoveRows();
    // 行号在删除点之后整体前移，重建索引最简单可靠
    rebuildIndex();
    emit countChanged();
}

PeerInfo PeerListModel::at(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return {};
    }
    return m_rows.at(row);
}

void PeerListModel::rebuildIndex()
{
    m_indexById.clear();
    for (int i = 0; i < m_rows.size(); ++i) {
        m_indexById.insert(m_rows.at(i).peerId, i);
    }
}

} // namespace FengSui
