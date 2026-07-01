// TransferListModel.cpp
// 传输任务列表模型实现。

#include "ui/viewmodels/TransferListModel.h"

namespace FengSui {

TransferListModel::TransferListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int TransferListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_rows.size();
}

double TransferListModel::progressOf(const TransferTask& t)
{
    if (t.fileSize <= 0) {
        return 0.0;
    }
    const double p = static_cast<double>(t.transferredBytes) / static_cast<double>(t.fileSize);
    return p < 0.0 ? 0.0 : (p > 1.0 ? 1.0 : p);
}

QVariant TransferListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row& r = m_rows.at(index.row());
    const TransferTask& t = r.task;
    switch (role) {
    case TransferIdRole:  return t.transferId;
    case DirectionRole:   return static_cast<int>(t.direction);
    case FileNameRole:    return t.fileName;
    case PeerNameRole:    return r.peerName;
    case StatusRole:      return static_cast<int>(t.status);
    case TransferredRole: return t.transferredBytes;
    case SizeRole:        return t.fileSize;
    case ProgressRole:    return progressOf(t);
    case ErrorRole:       return t.errorMessage;
    case CreatedAtRole:   return t.createdAt;
    case CompletedAtRole: return t.completedAt;
    default:              return {};
    }
}

QHash<int, QByteArray> TransferListModel::roleNames() const
{
    return {
        { TransferIdRole,  "transferId" },
        { DirectionRole,   "direction" },
        { FileNameRole,    "fileName" },
        { PeerNameRole,    "peerName" },
        { StatusRole,      "status" },
        { TransferredRole, "transferredBytes" },
        { SizeRole,        "fileSize" },
        { ProgressRole,    "progress" },
        { ErrorRole,       "errorMessage" },
        { CreatedAtRole,   "createdAt" },
        { CompletedAtRole, "completedAt" }
    };
}

void TransferListModel::resetRows(const QList<Row>& rows)
{
    beginResetModel();
    m_rows = rows;
    m_indexById.clear();
    for (int i = 0; i < m_rows.size(); ++i) {
        m_indexById.insert(m_rows.at(i).task.transferId, i);
    }
    endResetModel();
    emit countChanged();
}

void TransferListModel::upsert(const Row& row)
{
    const auto it = m_indexById.constFind(row.task.transferId);
    if (it != m_indexById.constEnd()) {
        const int r = it.value();
        m_rows[r] = row;
        const QModelIndex idx = index(r);
        emit dataChanged(idx, idx);   // 整行刷新
        return;
    }
    // 新任务插到最前（最新在上）
    beginInsertRows(QModelIndex(), 0, 0);
    m_rows.prepend(row);
    // 前插导致所有行号后移，重建索引
    m_indexById.clear();
    for (int i = 0; i < m_rows.size(); ++i) {
        m_indexById.insert(m_rows.at(i).task.transferId, i);
    }
    endInsertRows();
    emit countChanged();
}

QModelIndex TransferListModel::indexForId(const QString& transferId) const
{
    const auto it = m_indexById.constFind(transferId);
    if (it == m_indexById.constEnd()) {
        return {};
    }
    return index(it.value());
}

void TransferListModel::updateProgress(const QString& transferId, qint64 done, qint64 total)
{
    const QModelIndex idx = indexForId(transferId);
    if (!idx.isValid()) {
        return;
    }
    Row& r = m_rows[idx.row()];
    r.task.transferredBytes = done;
    if (total > 0) {
        r.task.fileSize = total;
    }
    // 只发进度相关 role，避免整行/整表刷新
    emit dataChanged(idx, idx, { TransferredRole, SizeRole, ProgressRole });
}

void TransferListModel::updateStatus(const QString& transferId, TransferStatus status,
                                     const QString& error)
{
    const QModelIndex idx = indexForId(transferId);
    if (!idx.isValid()) {
        return;
    }
    Row& r = m_rows[idx.row()];
    r.task.status = status;
    if (!error.isEmpty()) {
        r.task.errorMessage = error;
    }
    emit dataChanged(idx, idx, { StatusRole, ErrorRole });
}

} // namespace FengSui
