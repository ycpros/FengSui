// TransferViewModel.cpp
// 传输中心 ViewModel 实现。

#include "ui/viewmodels/TransferViewModel.h"

#include "core/CourierService.h"
#include "core/BeaconService.h"

namespace FengSui {

// ---- TransferFilterModel ----

TransferFilterModel::TransferFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &TransferFilterModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &TransferFilterModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &TransferFilterModel::countChanged);
}

void TransferFilterModel::setKeyword(const QString& kw)
{
    if (kw == m_keyword) {
        return;
    }
    m_keyword = kw;
    emit keywordChanged();
    invalidateFilter();
    emit countChanged();
}

void TransferFilterModel::setStatusFilter(int s)
{
    if (s == m_statusFilter) {
        return;
    }
    m_statusFilter = s;
    emit statusFilterChanged();
    invalidateFilter();
    emit countChanged();
}

bool TransferFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

    // 状态筛选
    if (m_statusFilter >= 0) {
        const int st = sourceModel()->data(idx, TransferListModel::StatusRole).toInt();
        if (st != m_statusFilter) {
            return false;
        }
    }

    // 关键词：匹配文件名或对端名（不区分大小写）
    if (!m_keyword.trimmed().isEmpty()) {
        const QString kw = m_keyword.trimmed();
        const QString file = sourceModel()->data(idx, TransferListModel::FileNameRole).toString();
        const QString peer = sourceModel()->data(idx, TransferListModel::PeerNameRole).toString();
        if (!file.contains(kw, Qt::CaseInsensitive) &&
            !peer.contains(kw, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

// ---- TransferViewModel ----

TransferViewModel::TransferViewModel(QObject* parent)
    : QObject(parent)
    , m_model(new TransferListModel(this))
    , m_proxy(new TransferFilterModel(this))
{
    m_proxy->setSourceModel(m_model);
}

void TransferViewModel::bind(CourierService* courier, BeaconService* beacon)
{
    if (m_courier) {
        disconnect(m_courier, nullptr, this, nullptr);
    }
    m_courier = courier;
    m_beacon = beacon;

    if (m_courier) {
        connect(m_courier, &CourierService::transferRequested,
                this, &TransferViewModel::onTransferRequested);
        connect(m_courier, &CourierService::transferProgress,
                this, &TransferViewModel::onTransferProgress);
        connect(m_courier, &CourierService::transferAccepted,
                this, &TransferViewModel::onTransferAccepted);
        connect(m_courier, &CourierService::transferRejected,
                this, &TransferViewModel::onTransferRejected);
        connect(m_courier, &CourierService::transferCompleted,
                this, &TransferViewModel::onTransferCompleted);
        connect(m_courier, &CourierService::transferFailed,
                this, &TransferViewModel::onTransferFailed);
        reload();
    }
}

QString TransferViewModel::resolvePeerName(const QString& peerId) const
{
    if (m_beacon) {
        const QList<PeerInfo> peers = m_beacon->peers();
        for (const PeerInfo& p : peers) {
            if (p.peerId == peerId) {
                return p.displayName.isEmpty() ? peerId : p.displayName;
            }
        }
    }
    return peerId;
}

TransferListModel::Row TransferViewModel::toRow(const TransferTask& task) const
{
    TransferListModel::Row row;
    row.task = task;
    row.peerName = resolvePeerName(task.peerId);
    return row;
}

void TransferViewModel::reload()
{
    if (!m_courier) {
        return;
    }
    const QList<TransferTask> tasks = m_courier->allTasks();
    QList<TransferListModel::Row> rows;
    rows.reserve(tasks.size());
    for (const TransferTask& t : tasks) {
        rows.append(toRow(t));
    }
    m_model->resetRows(rows);
}

void TransferViewModel::acceptTransfer(const QString& transferId)
{
    if (m_courier) {
        m_courier->acceptTransfer(transferId);
    }
}

void TransferViewModel::rejectTransfer(const QString& transferId)
{
    if (m_courier) {
        m_courier->rejectTransfer(transferId, QStringLiteral("用户拒绝"));
    }
}

void TransferViewModel::cancelTransfer(const QString& transferId)
{
    if (m_courier) {
        m_courier->cancelTransfer(transferId);
    }
}

void TransferViewModel::onTransferRequested(const TransferTask& task)
{
    m_model->upsert(toRow(task));
    // 通知 QML 弹出接受/拒绝对话框
    emit incomingRequest(task.transferId, resolvePeerName(task.peerId),
                         task.fileName, task.fileSize);
}

void TransferViewModel::onTransferProgress(const QString& id, qint64 done, qint64 total)
{
    // 高频信号：仅增量更新进度 role
    m_model->updateProgress(id, done, total);
}

void TransferViewModel::onTransferAccepted(const QString& id)
{
    m_model->updateStatus(id, TransferStatus::Transferring);
}

void TransferViewModel::onTransferRejected(const QString& id, const QString& reason)
{
    m_model->updateStatus(id, TransferStatus::Rejected, reason);
}

void TransferViewModel::onTransferCompleted(const QString& id)
{
    m_model->updateStatus(id, TransferStatus::Completed);
    // 完成后从仓库重新取该任务的最终字节数等（简单起见整体 reload 由页面按需触发）
}

void TransferViewModel::onTransferFailed(const QString& id, const QString& error)
{
    m_model->updateStatus(id, TransferStatus::Failed, error);
}

} // namespace FengSui
